#ifndef _Serialization_H
#define _Serialization_H

#include "types.h"

template <typename _T, int netsize>
inline u8 *SerializeInt(u8 *pos, _T val)
{
	for (int i=0;i<netsize;i++)
	{
		*(pos++)=val&0xff;
		val >>= 8;
	}
	return pos;
}

template <typename _T, int netsize>
inline _T DeserializeInt(u8 **pos)
{
	_T val=0;
	uint i=netsize;
	while (i--)
	{
		val = (val << 8) + (*pos)[i];
	}
	(*pos) += netsize;
	return val;
}

#endif
