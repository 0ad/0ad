#ifndef INCLUDED_NETWORK_STRINGCONVERTERS
#define INCLUDED_NETWORK_STRINGCONVERTERS

#include "ps/CStr.h"
#include "simulation/EntityHandles.h"

template <typename _T>
CStr NetMessageStringConvert(const _T &arg);

// String Converters
template <typename _T>
inline CStr NetMessageStringConvert(const _T &arg)
{
	return CStr(arg);
}

template <>
inline CStr NetMessageStringConvert(const HEntity &arg)
{
	return arg.operator CStr8();
}

#endif
