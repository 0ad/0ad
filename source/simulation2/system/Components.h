/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_COMPONENTS
#define INCLUDED_COMPONENTS

// Defaults for TypeList.h macros
#define MESSAGE(name)
#define INTERFACE(name)
#define COMPONENT(name)

#undef MESSAGE
#define MESSAGE(name) MT_##name,
enum EMessageTypeId
{
	MT__Invalid = 0,
#include "simulation2/TypeList.h"
	MT__LastNative
};
#undef MESSAGE
#define MESSAGE(name)

#undef INTERFACE
#define INTERFACE(name) IID_##name,
enum EInterfaceId
{
	IID__Invalid = 0,
#include "simulation2/TypeList.h"
	IID__LastNative
};
#undef INTERFACE
#define INTERFACE(name)

#undef COMPONENT
#define COMPONENT(name) CID_##name,
enum EComponentTypeId
{
	CID__Invalid = 0,
#include "simulation2/TypeList.h"
	CID__LastNative
};
#undef COMPONENT
#define COMPONENT(name)

#undef MESSAGE
#undef INTERFACE
#undef COMPONENT

#endif // INCLUDED_COMPONENTS
