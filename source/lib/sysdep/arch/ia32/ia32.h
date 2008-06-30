/**
 * =========================================================================
 * File        : ia32.h
 * Project     : 0 A.D.
 * Description : routines specific to IA-32
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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
