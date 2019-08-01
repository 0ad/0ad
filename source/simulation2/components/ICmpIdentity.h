/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_ICMPIDENTITY
#define INCLUDED_ICMPIDENTITY

#include "simulation2/system/Interface.h"

/**
 * Identity data.
 * (This interface only includes the functions needed by native code for entity selection)
 */
class ICmpIdentity : public IComponent
{
public:
	virtual std::string GetSelectionGroupName() = 0;

	virtual std::wstring GetPhenotype() = 0;

	DECLARE_INTERFACE_TYPE(Identity)
};

#endif // INCLUDED_ICMPIDENTITY
