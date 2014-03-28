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

#ifndef INCLUDED_AUTOROOTERS
#define INCLUDED_AUTOROOTERS

#include "scriptinterface/ScriptTypes.h"


/**
 * Helper for rooting large groups of script values.
 * Construct this object, push values into it, and they will all be rooted until this
 * object is destroyed.
 * Many of these objects can be used at once, but their lifetimes must be correctly nested.
 */
class AutoGCRooter
{
	NONCOPYABLE(AutoGCRooter);
public:
	AutoGCRooter(ScriptInterface& scriptInterface);
	~AutoGCRooter();

	void Push(JSObject* obj) { m_Objects.push_back(obj); }
	void Push(jsval val) { m_Vals.push_back(val); }
	void Push(JSIdArray* ida) { m_IdArrays.push_back(ida); }

	void Trace(JSTracer* trc);
private:
	ScriptInterface& m_ScriptInterface;
	AutoGCRooter* m_Previous;

	std::vector<JSObject*> m_Objects;
	std::vector<jsval> m_Vals;
	std::vector<JSIdArray*> m_IdArrays;
	// TODO: add vectors of other value types
};

#endif // INCLUDED_AUTOROOTERS
