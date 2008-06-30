/**
 * =========================================================================
 * File        : mahaf.h
 * Project     : 0 A.D.
 * Description : user-mode interface to Aken driver
 * =========================================================================
 */

// license: GPL; see lib/license.txt

// Mahaf - ferryman in Egyptian mythology that wakes up Aken,
// and the interface to the Aken driver.

#ifndef INCLUDED_MAHAF
#define INCLUDED_MAHAF

/**
 * @return whether mapping physical memory is known to be dangerous
 * on this platform.
 *
 * callable before or after mahaf_Init.
 *
 * note: mahaf_MapPhysicalMemory will complain if it
 * is called despite this function having returned true.
 **/
extern bool mahaf_IsPhysicalMappingDangerous();


extern bool mahaf_Init();
extern void mahaf_Shutdown();

extern u8  mahaf_ReadPort8 (u16 port);
extern u16 mahaf_ReadPort16(u16 port);
extern u32 mahaf_ReadPort32(u16 port);
extern void mahaf_WritePort8 (u16 port, u8  value);
extern void mahaf_WritePort16(u16 port, u16 value);
extern void mahaf_WritePort32(u16 port, u32 value);

extern volatile void* mahaf_MapPhysicalMemory(uintptr_t physicalAddress, size_t numBytes);
extern void mahaf_UnmapPhysicalMemory(volatile void* virtualAddress);

#endif	// INCLUDED_MAHAF
