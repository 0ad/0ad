/* Copyright (C) 2010 Wildfire Games.
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
 * Windows backend of os_cpu
 */

#ifndef INCLUDED_WCPU
#define INCLUDED_WCPU

#include "lib/sysdep/os/win/win.h"

extern Status wcpu_ReadFrequencyFromRegistry(u32& freqMhz);

// "affinity" and "processorNumber" are what Windows sees.
// "processorMask" and "processor" are the idealized representation we expose
// to users. the latter insulates them from process affinity restrictions by
// defining IDs as indices of the nonzero bits within the process affinity.
// these routines are provided for the benefit of wnuma.

extern DWORD_PTR wcpu_AffinityFromProcessorMask(DWORD_PTR processAffinity, uintptr_t processorMask);
extern uintptr_t wcpu_ProcessorMaskFromAffinity(DWORD_PTR processAffinity, DWORD_PTR affinity);

#endif	// #ifndef INCLUDED_WCPU
