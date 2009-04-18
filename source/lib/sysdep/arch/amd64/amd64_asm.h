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

/*
 * interface to various AMD64 functions (written in asm)
 */

#ifndef INCLUDED_AMD64_ASM
#define INCLUDED_AMD64_ASM

#ifdef __cplusplus
extern "C" {
#endif

struct x86_x64_CpuidRegs;
extern void CALL_CONV amd64_asm_cpuid(x86_x64_CpuidRegs* reg);

extern intptr_t CALL_CONV amd64_CAS(volatile uintptr_t *location, uintptr_t expected, uintptr_t newValue);

extern void CALL_CONV amd64_AtomicAdd(volatile intptr_t *location, intptr_t increment);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef INCLUDED_AMD64_ASM
