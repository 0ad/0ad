#ifndef __LAYEREDPAINTER_H__
#define __LAYEREDPAINTER_H__

#include "areapainter.h"
#include "terrain.h"

class LayeredPainter :
	public AreaPainter
{
private:
	std::vector<Terrain*> terrains;
	std::vector<int> widths;
public:
	LayeredPainter(const std::vector<Terrain*>& terrains, const std::vector<int>& widths);
	~LayeredPainter(void);

	virtual void paint(class Map* m, class Area* a);
};

#endif