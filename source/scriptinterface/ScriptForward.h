/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTFORWARD
#define INCLUDED_SCRIPTFORWARD


// Ignore some harmless warnings
#if GCC_VERSION
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#if CLANG_VERSION
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunused-parameter"
#endif
#if MSC_VERSION
# pragma warning(push, 1)
# pragma warning(disable: 4100)
#endif


#include "js/TypeDecls.h"

class ScriptInterface;
class ScriptRequest;

#if GCC_VERSION
# pragma GCC diagnostic pop
#endif
#if CLANG_VERSION
# pragma clang diagnostic pop
#endif
#if MSC_VERSION
# pragma warning(pop)
#endif


#endif // INCLUDED_SCRIPTFORWARD
