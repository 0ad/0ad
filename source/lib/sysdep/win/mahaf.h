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

extern bool MahafInit();
extern void MahafShutdown();

extern u8  ReadPort8 (u16 port);
extern u16 ReadPort16(u16 port);
extern u32 ReadPort32(u16 port);
extern void WritePort8 (u16 port, u8  value);
extern void WritePort16(u16 port, u16 value);
extern void WritePort32(u16 port, u32 value);

extern void* MapPhysicalMemory(uintptr_t physicalAddress, size_t numBytes);
extern void UnmapPhysicalMemory(void* virtualAddress);

#endif	// INCLUDED_MAHAF
