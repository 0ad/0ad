/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_PCH_WARNINGS
#define INCLUDED_PCH_WARNINGS

#include "lib/sysdep/compiler.h"	// MSC_VERSION

#if MSC_VERSION
// .. unimportant but not entirely senseless W4
# pragma warning(disable:4201)	// nameless struct (Matrix3D)
# pragma warning(disable:4244)	// conversion from uintN to uint8
// .. always disabled W4
# pragma warning(disable:4103)	// alignment changed after including header (boost has #pragma pack/pop in separate headers)
# pragma warning(disable:4127)	// conditional expression is constant; rationale: see STMT in lib.h.
# pragma warning(disable:4324)	// structure was padded due to __declspec(align())
# pragma warning(disable:4351)	// yes, default init of array entries is desired
# pragma warning(disable:4355)	// 'this' used in base member initializer list
# pragma warning(disable:4512)	// assignment operator could not be generated
# pragma warning(disable:4718)	// recursive call has no side effects, deleting
# pragma warning(disable:4786)	// identifier truncated to 255 chars
# pragma warning(disable:4996)	// function is deprecated
# pragma warning(disable:6011)	// dereferencing NULL pointer
# pragma warning(disable:6246)	// local declaration hides declaration of the same name in outer scope
# pragma warning(disable:6326)	// potential comparison of a constant with another constant
# pragma warning(disable:6334)	// sizeof operator applied to an expression with an operator might yield unexpected results
// .. Intel-specific
# if ICC_VERSION
#  pragma warning(disable:383)	// value copied to temporary, reference to temporary used
#  pragma warning(disable:981)	// operands are evaluated in unspecified order
#  pragma warning(disable:1418)	// external function definition with no prior declaration (raised for all non-static function templates)
#  pragma warning(disable:1572)	// floating-point equality and inequality comparisons are unreliable
#  pragma warning(disable:1684)	// conversion from pointer to same-sized integral type
#  pragma warning(disable:1786)	// function is deprecated (disabling 4996 isn't sufficient)
#  pragma warning(disable:2415)	// variable of static storage duration was declared but never referenced (raised by Boost)
# endif
// .. disabled by default in W4, ENABLE them
# pragma warning(default:4062)   // enumerator is not handled
# pragma warning(default:4254)   // [bit field] conversion, possible loss of data
# pragma warning(default:4265)   // class has virtual functions, but destructor is not virtual
# pragma warning(default:4296)   // [unsigned comparison vs. 0 =>] expression is always false
# pragma warning(default:4545 4546 4547 4549) // ill-formed comma expressions; exclude 4548 since _SECURE_SCL triggers it frequently
# pragma warning(default:4557)   // __assume contains effect
//# pragma warning(default:4710)   // function not inlined (often happens in STL)
# pragma warning(default:4836)   // local types or unnamed types cannot be used as template arguments
# pragma warning(default:4905)   // wide string literal cast to LPSTR
# pragma warning(default:4906)   // string literal cast to LPWSTR
# pragma warning(default:4928)   // illegal copy-initialization; more than one user-defined conversion has been implicitly applied
# pragma warning(default:4946)   // reinterpret_cast used between related classes
// .. disabled by default in W4, leave them that way
//# pragma warning(default:4191) // unsafe conversion (false alarms for function pointers)
//# pragma warning(default:4263) // member function does not override any base class virtual member function (happens in GUI)
//# pragma warning(default:4555) // expression has no effect (triggered by STL unused)
//# pragma warning(default:4619) // #pragma warning: there is no [such] warning number (false alarms in STL)
//# pragma warning(default:4668) // not defined as a preprocessor macro, replacing with '0' (frequent in Windows)
#endif

#endif	// #ifndef INCLUDED_PCH_WARNINGS
