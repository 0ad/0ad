/* Copyright (C) 2014 Wildfire Games.
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

#include "precompiled.h"

#include "ICmpRallyPointRenderer.h"
#include "simulation2/system/InterfaceScripted.h"

class CFixedVector2D;

BEGIN_INTERFACE_WRAPPER(RallyPointRenderer)
DEFINE_INTERFACE_METHOD_1("SetDisplayed", void, ICmpRallyPointRenderer, SetDisplayed, bool)
DEFINE_INTERFACE_METHOD_1("SetPosition", void, ICmpRallyPointRenderer, SetPosition, CFixedVector2D)
DEFINE_INTERFACE_METHOD_2("UpdatePosition", void, ICmpRallyPointRenderer, UpdatePosition, u32, CFixedVector2D)
DEFINE_INTERFACE_METHOD_1("AddPosition", void, ICmpRallyPointRenderer, AddPosition_wrapper, CFixedVector2D)
DEFINE_INTERFACE_METHOD_0("Reset", void, ICmpRallyPointRenderer, Reset)
DEFINE_INTERFACE_METHOD_0("IsSet", bool, ICmpRallyPointRenderer, IsSet)
END_INTERFACE_WRAPPER(RallyPointRenderer)
