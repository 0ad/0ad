/* Copyright (C) 2020 Wildfire Games.
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

#include "JSInterface_GUIProxy_impl.h"

#include "gui/ObjectTypes/CButton.h"

using GUIObjectType = CButton;

template<>
void JSI_GUIProxy<GUIObjectType>::CreateFunctions(const ScriptRequest& rq, GUIProxyProps* cache)
{
#define func(class, func) &JSInterface_GUIProxy::apply_to<GUIObjectType, class, &class::func>
	cache->setFunction(rq, "toString", func(IGUIObject, toString), 0);
	cache->setFunction(rq, "focus", func(IGUIObject, focus), 0);
	cache->setFunction(rq, "blur", func(IGUIObject, blur), 0);
	cache->setFunction(rq, "getComputedSize", func(IGUIObject, getComputedSize), 0);
	cache->setFunction(rq, "getTextSize", func(GUIObjectType, getTextSize), 0);
#undef func
}

template class JSI_GUIProxy<GUIObjectType>;
