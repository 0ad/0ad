/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * user-mode interface to Aken driver
 */

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
LIB_API bool mahaf_IsPhysicalMappingDangerous();


LIB_API Status mahaf_Init();
LIB_API void mahaf_Shutdown();

LIB_API u8  mahaf_ReadPort8 (u16 port);
LIB_API u16 mahaf_ReadPort16(u16 port);
LIB_API u32 mahaf_ReadPort32(u16 port);
LIB_API void mahaf_WritePort8 (u16 port, u8  value);
LIB_API void mahaf_WritePort16(u16 port, u16 value);
LIB_API void mahaf_WritePort32(u16 port, u32 value);

LIB_API volatile void* mahaf_MapPhysicalMemory(uintptr_t physicalAddress, size_t numBytes);
LIB_API void mahaf_UnmapPhysicalMemory(volatile void* virtualAddress);

LIB_API u64 mahaf_ReadModelSpecificRegister(u64 reg);
LIB_API void mahaf_WriteModelSpecificRegister(u64 reg, u64 value);

// must be done in the driver because Windows clears CR4.PCE[8]
LIB_API u64 mahaf_ReadPerformanceMonitoringCounter(u64 reg);

#endif	// INCLUDED_MAHAF
