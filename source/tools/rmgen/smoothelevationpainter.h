#ifndef __SMOOTHELEVATIONPAINTER_H__
#define __SMOOTHELEVATIONPAINTER_H__

#include "areapainter.h"
#include "terrain.h"
#include "map.h"
#include "area.h"

class SmoothElevationPainter :
	public AreaPainter
{
public:
	static const int SET = 0, MODIFY = 1;

private:
	int type;			// one of the constants above
	float elevation;		// target or delta
	int blendRadius;	// how many tiles to do blending for

	bool checkInArea(Map* m, Area* a, int x, int y);

public:
	SmoothElevationPainter(int type, float elevation, int blendRadius);
	~SmoothElevationPainter(void);

	virtual void paint(Map* m, Area* a);
};

#endif