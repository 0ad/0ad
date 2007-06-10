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

extern bool mahaf_Init();
extern void mahaf_Shutdown();

extern u8  mahaf_ReadPort8 (u16 port);
extern u16 mahaf_ReadPort16(u16 port);
extern u32 mahaf_ReadPort32(u16 port);
extern void mahaf_WritePort8 (u16 port, u8  value);
extern void mahaf_WritePort16(u16 port, u16 value);
extern void mahaf_WritePort32(u16 port, u32 value);

extern void* mahaf_MapPhysicalMemory(uintptr_t physicalAddress, size_t numBytes);
extern void mahaf_UnmapPhysicalMemory(void* virtualAddress);

/**
 * @return false on failure (insufficient paged pool?)
 **/
extern bool mahaf_CopyPhysicalMemory(uintptr_t physicalAddress, size_t numBytes, void* buffer);

#endif	// INCLUDED_MAHAF
