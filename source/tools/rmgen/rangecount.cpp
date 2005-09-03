#include "stdafx.h"
#include "rangecount.h"

RangeCount::RangeCount(int size) {
	nn = 1;
	while(nn < size) {
		nn *= 2;
	}
	vals = new int[2*nn];
	memset(vals, 0, 2*nn*sizeof(int));
}

RangeCount::~RangeCount() {
	delete[] vals;
}

int RangeCount::get(int pos) {
	return vals[nn + pos];
}

void RangeCount::set(int pos, int amt) {
	add(pos, amt-get(pos));
}

void RangeCount::add(int pos, int amt) {
	for(int s=nn; s>0; s/=2) {
		vals[s + pos] += amt;
		pos /= 2;
	}
}

int RangeCount::get(int start, int end) {
	int ret = 0;
	int i;
	for(i=1; start+i<=end; i*=2) {
		if(start & i) {
			ret += vals[nn/i + start/i];
			start += i;
		}
	}
	while(i) {
		if(start+i <= end) {
			ret += vals[nn/i + start/i];
			start += i;
		}
		i /= 2;
	}
	return ret;
}