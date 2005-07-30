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
			float x = cx + distance*cos(ang);
			float y = cy + distance*sin(ang);
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

bool SimpleGroup::place(Map* m, Constraint* constr, vector<Object*>& ret) {
	for(int i=0; i<elements.size(); i++) {
		if(!elements[i]->place(x, y, m, constr, ret)) {
			return false;
		}
	}
	return true;
}

SimpleGroup::SimpleGroup(vector<SimpleGroup::Element*>& e, int _x, int _y):
	elements(e), x(_x), y(_y)	
{
}

SimpleGroup::~SimpleGroup(void) {
	for(int i=0; i<elements.size(); i++) {
		delete elements[i];
	}
}