/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_ICMPVALUEMODIFICATIONMANAGER
#define INCLUDED_ICMPVALUEMODIFICATIONMANAGER

#include "simulation2/system/Interface.h"

#include "maths/Fixed.h"

/**
 * value modification manager interface.
 * (This interface only includes the functions needed by native code for accessing
 *	value modification data, the associated logic is handled in scripts)
 */
class ICmpValueModificationManager : public IComponent
{
public:
	virtual fixed ApplyModifications(std::wstring valueName, fixed currentValue, entity_id_t entity) = 0;
	virtual u32 ApplyModifications(std::wstring valueName, u32 currentValue, entity_id_t entity) = 0;
	virtual std::wstring ApplyModifications(std::wstring valueName, std::wstring currentValue, entity_id_t entity) = 0;

	DECLARE_INTERFACE_TYPE(ValueModificationManager)
};

#endif // INCLUDED_ICMPVALUEMODIFICATIONMANAGER
