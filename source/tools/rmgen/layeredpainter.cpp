#include "stdafx.h"
#include "layeredpainter.h"
#include "area.h"
#include "terrain.h"
#include "map.h"
#include "pointmap.h"

using namespace std;

LayeredPainter::LayeredPainter(const vector<Terrain*>& ts, const vector<int>& ws)
{
	terrains = ts;
	widths = ws;
}

LayeredPainter::~LayeredPainter(void)
{
}

void LayeredPainter::paint(Map* m, Area* a) {
	PointMap<int> saw;
	PointMap<int> dist;

	queue<Point> q;

	// push edge points
	vector<Point>& pts = a->points;
	for(int i=0; i<pts.size(); i++) {
		int x = pts[i].x, y = pts[i].y;
		for(int dx=-1; dx<=1; dx++) {
			for(int dy=-1; dy<=1; dy++) {
				int nx = x+dx, ny = y+dy;
				Point np(nx, ny);
				if(m->validT(nx, ny) && m->area[nx][ny]!=a && !saw[np]) {
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
		if(m->area[p.x][p.y] == a) {
			int w=0, i=0;
			for(; i<widths.size(); i++) {
				w += widths[i];
				if(w>=d) {
					break;
				}
			}
			terrains[i]->place(m, p.x, p.y);
		}

		// enqueue neighbours
		for(int dx=-1; dx<=1; dx++) {
			for(int dy=-1; dy<=1; dy++) {
				int nx = p.x+dx, ny = p.y+dy;
				Point np(nx, ny);
				if(m->validT(nx, ny) && m->area[nx][ny]==a && !saw[np]) {
					saw[np] = 1;
					dist[np] = d+1;
					q.push(np);
				}
			}
		}
	}
}