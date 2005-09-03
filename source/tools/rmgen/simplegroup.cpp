#include "stdafx.h"
#include "simplegroup.h"
#include "point.h"
#include "random.h"
#include "map.h"

using namespace std;

SimpleGroup::Element::Element(){
}

SimpleGroup::Element::Element(const std::string& t, int c, float d):
	type(t), count(c), distance(d)
{
}

SimpleGroup::Element::~Element() {
}

bool SimpleGroup::Element::place(int cx, int cy, Map* m, Constraint* constr, vector<Object*>& ret) {
	int failCount = 0;
	for(int i=0; i<count; i++) {
		while(true) {
			float ang = RandFloat()*2*PI;
			float x = cx + distance*cos(ang) + 0.5f;
			float y = cy + distance*sin(ang) + 0.5f;
			int ix = (int) x;
			int iy = (int) y;
			if(m->validT(ix, iy) && constr->allows(m, ix, iy)) {
				ret.push_back(new Object(type, 0, x, 0, y, RandFloat()*2*PI));
				break;
			}
			else {
				failCount++;
				if(failCount > 20) {
					return false;
				}
			}
		}
	}
	return true;
}

bool SimpleGroup::place(Map* m, Constraint* constr) {
	vector<Object*> ret;
	for(int i=0; i<elements.size(); i++) {
		if(!elements[i]->place(x, y, m, constr, ret)) {
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

SimpleGroup::SimpleGroup(vector<SimpleGroup::Element*>& e, TileClass* tc, int _x, int _y):
	elements(e), x(_x), y(_y), tileClass(tc)	
{
}

SimpleGroup::~SimpleGroup(void) {
	for(int i=0; i<elements.size(); i++) {
		delete elements[i];
	}
}