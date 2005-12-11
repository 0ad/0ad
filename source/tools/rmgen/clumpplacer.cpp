#include "stdafx.h"
#include "clumpplacer.h"
#include "random.h"
#include "pointmap.h"

using namespace std;

ClumpPlacer::ClumpPlacer(float size, float coherence, float smoothness, float failFraction, int x, int y)
{
	this->size = size;
	this->coherence = coherence;
	this->smoothness = smoothness;
	this->x = x;
	this->y = y;
	this->failFraction = failFraction;
}

ClumpPlacer::~ClumpPlacer()
{
}

bool ClumpPlacer::place(class Map* m, Constraint* constr, std::vector<Point>& retVec) {
	if(!m->validT(x, y) || !constr->allows(m, x, y)) {
		return false;
	}

	PointMap<int> gotRet;

	float radius = sqrt(size / PI);
	float perim = 4 * radius * 2 * PI;
	int intPerim = (int)(ceil(perim));
	vector<float> noise(intPerim);

	int ctrlPts = 1 + (int)(1.0/max(smoothness,1.0f/intPerim));
	if(ctrlPts > radius * 2 * PI) ctrlPts = (int) (radius * 2 * PI) + 1;
	vector<float> ctrlCoords(ctrlPts+1);
	vector<float> ctrlVals(ctrlPts+1);
	for(int i=0; i<ctrlPts; i++) {
		ctrlCoords[i] = i * perim / ctrlPts;
		ctrlVals[i] = 2.0*RandFloat();
	}

	int c = 0;
	int looped = 0;
	for(int i=0; i<intPerim; i++) {
		if(ctrlCoords[(c+1) % ctrlPts] < i && !looped) {
			c = (c+1) % ctrlPts;
			if(c==ctrlPts-1) looped = 1;
		}
		float t = (i - ctrlCoords[c]) / ((looped ? perim : ctrlCoords[(c+1)%ctrlPts]) - ctrlCoords[c]);
		float v0 = ctrlVals[(c+ctrlPts-1)%ctrlPts];
		float v1 = ctrlVals[c];
		float v2 = ctrlVals[(c+1)%ctrlPts];
		float v3 = ctrlVals[(c+2)%ctrlPts];
		float P = (v3 - v2) - (v0 - v1);
		float Q = (v0 - v1) - P;
		float R = v2 - v0;
		float S = v1;
		noise[i] = P*t*t*t + Q*t*t + R*t + S;
	}

	int failed = 0;
	for(int p=0; p<intPerim; p++) {
		float th = 2 * PI * p / perim;
		float r = radius * (1 + (1-coherence)*noise[p]);
		float s = sin(th);
		float c = cos(th);
		float xx=x, yy=y;
		for(float k=0; k<r; k++) {
			int i = (int)xx, j = (int)yy;
			if(m->validT(i, j) && constr->allows(m, i, j)) {
				Point p(i,j);
				if(!gotRet[p]) {
					gotRet[p] = 1;
					retVec.push_back(p);
				}
			}
			else {
				failed++;
			}
			xx += s;
			yy += c;
		}
	}

	return (failed > size*failFraction ? false : true);
}