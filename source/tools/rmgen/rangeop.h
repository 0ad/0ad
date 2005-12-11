#ifndef __RANGEOP_H__
#define __RANGEOP_H__

class RangeOp {
private:
	int* vals;	// has size 2*nn
	int nn;	// smallest power of 2 >= size
public:
	RangeOp(int size);
	~RangeOp();
	void set(int pos, int amt);
	void add(int pos, int amt);
	int get(int pos);
	int get(int start, int end);
};

#endif
