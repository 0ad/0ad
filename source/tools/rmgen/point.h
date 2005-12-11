#ifndef __POINT_H_
#define __POINT_H_

class Point
{
public:
	int x, y;
	Point(void);
	Point(int x, int y);
	~Point(void);
	bool operator<(const Point& p) const;
	bool operator==(const Point& p) const;
	operator size_t() const;	// cheap hack to provide a hash function
};

#endif