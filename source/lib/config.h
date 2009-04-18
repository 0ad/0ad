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
 * compile-time configuration for the entire project
 */

#ifndef INCLUDED_CONFIG
#define INCLUDED_CONFIG

// notes:
// - this file is included in the PCH and thus affects the entire project.
//   to avoid unnecessary full rebuilds, place settings of more limited
//   applicability in config2.h and explicitly include that header.
// - config macros are always defined; their values (1 or 0) are tested
//   with #if instead of #ifdef. this protects against typos by at least
//   causing a warning if the tested macro is undefined.
// - allow override via compiler settings by checking #ifndef.

// pre-compiled headers
#ifndef CONFIG_ENABLE_PCH
# define CONFIG_ENABLE_PCH 1	// improve build performance
#endif

// frame pointers
#ifndef CONFIG_OMIT_FP
# ifdef NDEBUG
#  define CONFIG_OMIT_FP 1	// improve performance
# else
#  define CONFIG_OMIT_FP 0	// enable use of ia32's fast stack walk
# endif
#endif

// try to prevent any exceptions from being thrown - even by the C++
// standard library. useful only for performance tests.
#ifndef CONFIG_DISABLE_EXCEPTIONS
# define CONFIG_DISABLE_EXCEPTIONS 0
#endif

// enable additional debug checks (very slow).
#ifndef CONFIG_PARANOIA
# define CONFIG_PARANOIA 0
#endif

// final release; disables some safety checks.
#ifndef CONFIG_FINAL
# define CONFIG_FINAL 0
#endif

#endif	// #ifndef INCLUDED_CONFIG
