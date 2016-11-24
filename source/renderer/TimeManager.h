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

#ifndef INCLUDED_TIMEMANAGER
#define INCLUDED_TIMEMANAGER

#include "graphics/Texture.h"
#include "maths/Matrix3D.h"
#include "lib/ogl.h"


class CTimeManager
{
public:
	CTimeManager();

	double GetFrameDelta();
	double GetGlobalTime();

	void Update(double delta);

private:
	double m_frameDelta;
	double m_globalTime;
};


#endif // INCLUDED_TIMEMANAGER
