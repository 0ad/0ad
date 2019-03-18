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

#ifndef INCLUDED_JSI_IGUIOBJECT
#define INCLUDED_JSI_IGUIOBJECT

#include "scriptinterface/ScriptInterface.h"

namespace JSI_IGUIObject
{
	extern JSClass JSI_class;
	extern JSFunctionSpec JSI_methods[];
	bool getProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp);
	bool setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool UNUSED(strict), JS::MutableHandleValue vp);
	bool construct(JSContext* cx, uint argc, JS::Value* vp);
	bool toString(JSContext* cx, uint argc, JS::Value* vp);
	bool focus(JSContext* cx, uint argc, JS::Value* vp);
	bool blur(JSContext* cx, uint argc, JS::Value* vp);
	bool getComputedSize(JSContext* cx, uint argc, JS::Value* vp);
	bool getTextSize(JSContext* cx, uint argc, JS::Value* vp);
	void init(ScriptInterface& scriptInterface);
}

#endif // INCLUDED_JSI_IGUIOBJECT
