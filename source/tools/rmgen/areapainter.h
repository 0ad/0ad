#ifndef __AREAPAINTER_H__
#define __AREAPAINTER_H__

class AreaPainter
{
public:
	virtual void paint(class Map* m, class Area* a) = 0;

	AreaPainter(void);
	virtual ~AreaPainter(void);
};

#endif