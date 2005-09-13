#include "stdafx.h"
#include "simplegroup.h"
#include "point.h"
#include "random.h"
#include "map.h"
#include "rmgen.h"

using namespace std;

SimpleGroup::Element::Element(){
}

SimpleGroup::Element::Element(const std::string& t, int minC, int maxC, float minD, float maxD):
	type(t), minCount(minC), maxCount(maxC), minDistance(minD), maxDistance(maxD)
{
	if(minCount > maxCount) {
		JS_ReportError(cx, "SimpleObject: minCount must be less than or equal to maxCount");
	}
	if(minDistance > maxDistance) {
		JS_ReportError(cx, "SimpleObject: minDistance must be less than or equal to maxDistance");
	}
}

SimpleGroup::Element::~Element() {
}

bool SimpleGroup::Element::place(int cx, int cy, Map* m, int player, bool avoidSelf,
								 Constraint* constr, vector<Object*>& ret) {
	int failCount = 0;
	int count = RandInt(minCount, maxCount);
	for(int i=0; i<count; i++) {
		while(true) {
			float distance = RandFloat(minDistance, maxDistance);
			float angle = RandFloat(0, 2*PI);

			float x = cx + 0.5f + distance*cos(angle);
			float y = cy + 0.5f + distance*sin(angle);

			if(x<0 || y<0 || x>m->size || y>m->size) {
				goto bad;
			}

			if(avoidSelf) {
				for(int i=0; i<ret.size(); i++) {
					float dx = x - ret[i]->x;
					float dy = y - ret[i]->y;
					if(dx*dx + dy*dy < 1) {
						goto bad;
					}
				}
			}

			if(!constr->allows(m, (int)x, (int)y)) {
				goto bad;
			}

			// if we got here, we're good
			ret.push_back(new Object(type, player, x, y, RandFloat()*2*PI));
			break;

bad:		failCount++;
			if(failCount > 20) {
				return false;
			}
		}
	}
	return true;
}

bool SimpleGroup::place(Map* m, int player, Constraint* constr) {
	vector<Object*> ret;
	for(int i=0; i<elements.size(); i++) {
		if(!elements[i]->place(x, y, m, player, avoidSelf, constr, ret)) {
			return false;
		}
	}
	for(int i=0; i<ret.size(); i++) {
		m->addObject(ret[i]);
		if(tileClass != 0) {
			tileClass->add((int) ret[i]->x, (int) ret[i]->y);
		}
	}
	return true;
}

SimpleGroup::SimpleGroup(vector<SimpleGroup::Element*>& e, TileClass* tc, bool as, int _x, int _y):
	elements(e), x(_x), y(_y), tileClass(tc), avoidSelf(as)
{
}

SimpleGroup::~SimpleGroup(void) {
	for(int i=0; i<elements.size(); i++) {
		delete elements[i];
	}
}