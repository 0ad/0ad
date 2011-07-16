/* Copyright (C) 2009 Wildfire Games.
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

// JSInterface_Entity.h
//
// 
// A JavaScript class representing a Pyrogenesis CVector3D object.
//
// Usage: Used when manipulating objects of class 'Vector3D' in JavaScript.

#include "scripting/ScriptingHost.h"
#include "maths/Vector3D.h"

#ifndef INCLUDED_JSI_VECTOR3
#define INCLUDED_JSI_VECTOR3

namespace JSI_Vector3D
{
	enum 
	{
		component_x,
		component_y,
		component_z
	};
	JSBool toString(JSContext* cx, uintN argc, jsval* vp);

	struct Vector3D_Info
	{
		IPropertyOwner* owner;
		void (IPropertyOwner::*updateFn)();
		void (IPropertyOwner::*freshenFn)();
		CVector3D* vector;
		Vector3D_Info();
		Vector3D_Info(float x, float y, float z);
		Vector3D_Info(const CVector3D& copy);
		Vector3D_Info(CVector3D* attach, IPropertyOwner* _owner);
		Vector3D_Info(CVector3D* attach, IPropertyOwner* _owner, void(IPropertyOwner::*_updateFn)(void));
		Vector3D_Info(CVector3D* attach, IPropertyOwner* _owner, void(IPropertyOwner::*_updateFn)(void), void(IPropertyOwner::*_freshenFn)(void));
		~Vector3D_Info();
	};
	extern JSClass JSI_class;
	extern JSPropertySpec JSI_props[];
	extern JSFunctionSpec JSI_methods[];

	JSBool getProperty(JSContext* cx, JSObject* obj, jsid id, jsval* vp);
	JSBool setProperty(JSContext* cx, JSObject* obj, jsid id, JSBool strict, jsval* vp);
	void finalize(JSContext* cx, JSObject* obj);
	JSBool construct(JSContext* cx, uintN argc, jsval* vp);
	void init();
}

#endif
