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
 * File        : ia32.h
 * Project     : 0 A.D.
 * Description : routines specific to IA-32
 * =========================================================================
 */

#ifndef INCLUDED_IA32
#define INCLUDED_IA32

#if !ARCH_IA32
# error "including ia32.h without ARCH_IA32=1"
#endif

/**
 * check if there is an IA-32 CALL instruction right before ret_addr.
 * @return INFO::OK if so and ERR::FAIL if not.
 *
 * also attempts to determine the call target. if that is possible
 * (directly addressed relative or indirect jumps), it is stored in
 * target, which is otherwise 0.
 *
 * this function is used for walking the call stack.
 **/
LIB_API LibError ia32_GetCallTarget(void* ret_addr, void** target);

#endif	// #ifndef INCLUDED_IA32
