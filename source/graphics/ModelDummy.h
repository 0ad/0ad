/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_MODELDUMMY
#define INCLUDED_MODELDUMMY

#include "graphics/ModelAbstract.h"

/**
 * Empty placeholder ModelAbstract implementation, to render nothing.
 */
class CModelDummy final : public CModelAbstract
{
	NONCOPYABLE(CModelDummy);

public:

	CModelDummy() = default;
	virtual ~CModelDummy() = default;

	virtual CModelAbstract* Clone() const { return new CModelDummy(); }
	virtual CModelDummy* ToCModelDummy() { return this; }

	virtual void CalcBounds() {};
	virtual void SetTerrainDirty(ssize_t, ssize_t, ssize_t, ssize_t) {}
	virtual void ValidatePosition() {};
	virtual void InvalidatePosition() {};
};

#endif // INCLUDED_MODELDUMMY
