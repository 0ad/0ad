// (included from precompiled.h)

#include "lib/sysdep/compiler.h"	// MSC_VERSION

#if MSC_VERSION
// .. temporarily disabled W4 (unimportant but ought to be fixed)
# pragma warning(disable:4201)	// nameless struct (Matrix3D)
# pragma warning(disable:4244)	// conversion from uintN to uint8
// .. permanently disabled W4
# pragma warning(disable:4103)	// alignment changed after including header (boost has #pragma pack/pop in separate headers)
# pragma warning(disable:4127)	// conditional expression is constant; rationale: see STMT in lib.h.
# pragma warning(disable:4351)	// yes, default init of array entries is desired
# pragma warning(disable:4355)	// 'this' used in base member initializer list
# pragma warning(disable:4718)	// recursive call has no side effects, deleting
# pragma warning(disable:4786)	// identifier truncated to 255 chars
# pragma warning(disable:4996)	// function is deprecated
// .. disabled only for the precompiled headers
# pragma warning(disable:4702)	// unreachable code (frequent in STL)
// .. VS2005 code analysis (very frequent ones)
# if MSC_VERSION >= 1400
#  pragma warning(disable:6011)	// dereferencing NULL pointer
#  pragma warning(disable:6246)	// local declaration hides declaration of the same name in outer scope
# endif
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
// .. ENABLE ones disabled even in W4
# pragma warning(default:4062)   // enumerator is not handled
//# pragma warning(default:4191) // unsafe conversion (false alarms for function pointers)
# pragma warning(default:4254)   // [bit field] conversion, possible loss of data
//# pragma warning(default:4263) // member function does not override any base class virtual member function (happens in GUI)
# pragma warning(default:4265)   // class has virtual functions, but destructor is not virtual
# pragma warning(default:4296)   // [unsigned comparison vs. 0 =>] expression is always false
# pragma warning(default:4545 4546 4547 4548 4549) // ill-formed comma expressions
//# pragma warning(default:4555) // expression has no effect (triggered by STL unused)
# pragma warning(default:4557)   // __assume contains effect
//# pragma warning(default:4619) // #pragma warning: there is no [such] warning number (false alarms in STL)
//# pragma warning(default:4668) // not defined as a preprocessor macro, replacing with '0' (frequent in Windows)
# pragma warning(default:4710)   // function not inlined
# pragma warning(default:4836)   // local types or unnamed types cannot be used as template arguments
# pragma warning(default:4905)   // wide string literal cast to LPSTR
# pragma warning(default:4906)   // string literal cast to LPWSTR
# pragma warning(default:4928)   // illegal copy-initialization; more than one user-defined conversion has been implicitly applied
# pragma warning(default:4946)   // reinterpret_cast used between related classes
#endif
