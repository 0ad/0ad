#ifndef __POINTMAP_H__
#define __POINTMAP_H__

#include "point.h"

// A 2D map class; should probably be optimized more, but beats STL map or hash_map
template<typename E> class PointMap
{
	static const int T = 5, P = 5;
	static const int T_VAL = (1<<T), P_VAL = (1<<P);
	static const int T_MASK = (1<<T)-1, P_MASK = (1<<P)-1;
	E** patches;
public:
	PointMap(void)
	{
		patches = new E*[1<<(P+P)];
		memset(patches, 0, (1<<(P+P)) * sizeof(E*));
	}

	~PointMap(void)
	{
		for(int i=0; i < (1<<(P+P)); i++) 
		{
			if(patches[i] != 0) 
			{
				delete[] patches[i];
			}
		}
		delete[] patches;
	}

	E& operator[] (Point p)
	{
		int patchIndex = (p.x >> T) * P_VAL + (p.y >> T);
		int tileIndex = (p.x & T_MASK) * T_VAL + (p.y & T_MASK);

		if(patches[patchIndex] == 0) 
		{
			patches[patchIndex] = new E[1<<(T+T)];
			memset(patches[patchIndex], 0, (1<<(T+T)) * sizeof(E));
		}

		return patches[patchIndex][tileIndex];
	}
};

#endif