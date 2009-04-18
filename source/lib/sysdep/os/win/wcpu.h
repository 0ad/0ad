/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : wcpu.h
 * Project     : 0 A.D.
 * Description : Windows backend of os_cpu
 * =========================================================================
 */

#ifndef INCLUDED_WCPU
#define INCLUDED_WCPU

#include "win.h"

// "affinity" and "processorNumber" are what Windows sees.
// "processorMask" and "processor" are the idealized representation we expose
// to users. the latter insulates them from process affinity restrictions by
// defining IDs as indices of the nonzero bits within the process affinity.
// these routines are provided for the benefit of wnuma.

extern DWORD_PTR wcpu_AffinityFromProcessorMask(DWORD_PTR processAffinity, uintptr_t processorMask);
extern uintptr_t wcpu_ProcessorMaskFromAffinity(DWORD_PTR processAffinity, DWORD_PTR affinity);

#endif	// #ifndef INCLUDED_WCPU
