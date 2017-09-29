/* Copyright (C) 2017 Wildfire Games.
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

#include "ICmpSoundManager.h"

#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(SoundManager)
DEFINE_INTERFACE_METHOD_2("PlaySoundGroup", void, ICmpSoundManager, PlaySoundGroup, std::wstring, entity_id_t)
DEFINE_INTERFACE_METHOD_2("PlaySoundGroupAtPosition", void, ICmpSoundManager, PlaySoundGroupAtPosition, std::wstring, CFixedVector3D)
DEFINE_INTERFACE_METHOD_0("StopMusic", void, ICmpSoundManager, StopMusic)
END_INTERFACE_WRAPPER(SoundManager)
