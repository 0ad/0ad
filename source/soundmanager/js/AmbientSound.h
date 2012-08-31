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

#ifndef INCLUDED_JAMBIENTSOUND
#define INCLUDED_JAMBIENTSOUND

#include "scripting/ScriptableObject.h"
#include "soundmanager/items/ISoundItem.h"

class JAmbientSound : public CJSObject<JAmbientSound>
{
public:	
	JAmbientSound(const VfsPath& pathname);
	
	CStr ToString(JSContext* cx, uintN argc, jsval* argv);
	
	bool Play(JSContext* cx, uintN argc, jsval* argv);
	
	bool Loop(JSContext* cx, uintN argc, jsval* argv);
	bool Free(JSContext* cx, uintN argc, jsval* argv);

	bool SetGain(JSContext* cx, uintN argc, jsval* argv);
	bool SetPitch(JSContext* cx, uintN argc, jsval* argv);	
	bool Fade(JSContext* cx, uintN argc, jsval* argv);
	
	static JSBool Construct(JSContext* cx, uintN argc, jsval* vp);
	void clearSoundItem();
	static void ScriptingInit();
protected:
	
	VfsPath m_FileName;

};

#endif	// #ifndef INCLUDED_JAMBIENTSOUND

