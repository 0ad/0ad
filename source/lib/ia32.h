#ifndef _M_IX86
#define __IA32_H__
#endif


#ifndef __IA32_H__

#include "types.h"


extern u64 rdtsc();


#ifndef _MCW_PC
#define _MCW_PC		0x0300		// Precision Control
#endif
#ifndef _PC_24
#define	_PC_24		0x0000		// 24 bits
#endif

extern uint _control87(uint new_cw, uint mask);

#endif	// #ifndef __IA32_H__
