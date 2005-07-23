#include "stdafx.h"
#include "rmgen.h"
#include "smoothelevationpainter.h"

using namespace std;

SmoothElevationPainter::SmoothElevationPainter(int t, float e, int b)
{
	type = t;
	elevation = e;
	blendRadius = b;
	if(type!=SET && type!=MODIFY) {
		JS_ReportError(cx, "SmoothElevationPainter: type must be either SET or MODIFY");
	}
}

SmoothElevationPainter::~SmoothElevationPainter(void)
{
}

bool SmoothElevationPainter::checkInArea(Map* m, Area* a, int x, int y) {
	if(m->validT(x, y)) {
		return m->area[x][y] == a;
	}
	else {
		return false;
	}
}

void SmoothElevationPainter::paint(Map* m, Area* a) {
	map<Point, int> saw;
	map<Point, int> dist;

	queue<Point> q;

	vector<Point>& pts = a->points;

	set<Point> heightPts;
	map<Point, float> newHeight;

	// get a list of all points
	for(int i=0; i<pts.size(); i++) {
		int x = pts[i].x, y = pts[i].y;
		for(int dx=-1; dx<=2; dx++) {
			for(int dy=-1; dy<=2; dy++) {
				int nx = x+dx, ny = y+dy;
				Point np(nx, ny);
				if(m->validH(nx, ny)) {
					heightPts.insert(np);
					newHeight[np] = m->height[nx][ny];
				}
			}
		}
	}

	// push edge points
	for(int i=0; i<pts.size(); i++) {
		int x = pts[i].x, y = pts[i].y;
		for(int dx=-1; dx<=2; dx++) {
			for(int dy=-1; dy<=2; dy++) {
				int nx = x+dx, ny = y+dy;
				Point np(nx, ny);
				if(m->validH(nx, ny) 
						&& !checkInArea(m, a, nx, ny)
						&& !checkInArea(m, a, nx-1, ny)
						&& !checkInArea(m, a, nx, ny-1)
						&& !checkInArea(m, a, nx-1, ny-1)
						&& !saw[np]) {
					saw[np] = 1;
					dist[np] = 0;
					q.push(np);
				}
			}
		}
	}

	// do BFS inwards to find distances to edge
	while(!q.empty()) {
		Point p = q.front();
		q.pop();

		int d = dist[p];

		// paint if in area
		if(m->validH(p.x, p.y)
				&& (checkInArea(m, a, p.x, p.y) || checkInArea(m, a, p.x-1, p.y) 
				|| checkInArea(m, a, p.x, p.y-1) || checkInArea(m, a, p.x-1, p.y-1))) {
			if(d <= blendRadius) {
				float a = ((float)(d-1)) / blendRadius;
				if(type == SET) {
					newHeight[p] = a*elevation + (1-a)*m->height[p.x][p.y];
				}
				else {	// type == MODIFY
					newHeight[p] += a*elevation;
				}
			}
			else {	// also happens when blendRadius == 0
				if(type == SET) {
					newHeight[p] = elevation;
				}
				else {	// type == MODIFY
					newHeight[p] += elevation;
				}
			}
		}

		// enqueue neighbours
		for(int dx=-1; dx<=1; dx++) {
			for(int dy=-1; dy<=1; dy++) {
				int nx = p.x+dx, ny = p.y+dy;
				Point np(nx, ny);
				if(m->validH(nx, ny) 
						&& (checkInArea(m, a, nx, ny) || checkInArea(m, a, nx-1, ny) 
							|| checkInArea(m, a, nx, ny-1) || checkInArea(m, a, nx-1, ny-1))
						&& !saw[np]) {
					saw[np] = 1;
					dist[np] = d+1;
					q.push(np);
				}
			}
		}
	}

	// smooth everything out
	for(set<Point>::iterator it = heightPts.begin(); it != heightPts.end(); it++) {
		Point p = *it;
		int x = p.x, y = p.y;
		if((checkInArea(m, a, x, y) || checkInArea(m, a, x-1, y) 
			|| checkInArea(m, a, x, y-1) || checkInArea(m, a, x-1, y-1))) {
			float sum = 8 * newHeight[p];
			int count = 8;
			for(int dx=-1; dx<=1; dx++) {
				for(int dy=-1; dy<=1; dy++) {
					int nx = x+dx, ny = y+dy;
					if(m->validH(nx, ny)) {
						sum += m->height[nx][ny];
						count++;
					}
				}
			}
			m->height[x][y] = sum/count;
		}
	}
	// don't smooth
	/*
	for(int i=0; i<heightPts.size(); i++) {
		int x = heightPts[i].x, y = heightPts[i].y;
		if((checkInArea(m, a, x, y) || checkInArea(m, a, x-1, y) 
			|| checkInArea(m, a, x, y-1) || checkInArea(m, a, x-1, y-1))) {
			m->height[x][y] = newHeight[p];
		}
	*/
}