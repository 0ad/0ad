#ifndef __RANGECOUNT_H__
#define __RANGECOUNT_H__

class RangeCount {
private:
	int* vals;	// has size 2*nn
	int nn;	// smallest power of 2 >= size
public:
	RangeCount(int size);
	~RangeCount();
	void set(int pos, int amt);
	void add(int pos, int amt);
	int get(int pos);
	int get(int start, int end);
};

#endif
