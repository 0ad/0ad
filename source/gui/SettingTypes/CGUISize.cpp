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

#include "precompiled.h"

#include "CGUISize.h"

#include "gui/Scripting/JSInterface_GUISize.h"
#include "ps/CLogger.h"

CGUISize::CGUISize()
	: pixel(), percent()
{
}

CGUISize::CGUISize(const CRect& pixel, const CRect& percent)
	: pixel(pixel), percent(percent)
{
}

CGUISize CGUISize::Full()
{
	return CGUISize(CRect(0, 0, 0, 0), CRect(0, 0, 100, 100));
}

CRect CGUISize::GetSize(const CRect& parent) const
{
	// If it's a 0 0 100% 100% we need no calculations
	if (percent == CRect(0.f, 0.f, 100.f, 100.f) && pixel == CRect())
		return parent;

	CRect client;

	// This should probably be cached and not calculated all the time for every object.
    client.left =	parent.left + (parent.right-parent.left)*percent.left/100.f + pixel.left;
	client.top =	parent.top + (parent.bottom-parent.top)*percent.top/100.f + pixel.top;
	client.right =	parent.left + (parent.right-parent.left)*percent.right/100.f + pixel.right;
	client.bottom =	parent.top + (parent.bottom-parent.top)*percent.bottom/100.f + pixel.bottom;

	return client;
}

bool CGUISize::FromString(const CStr& Value)
{
	/*
	 * GUISizes contain a left, top, right, and bottom
	 *  for example: "50%-150 10%+9 50%+150 10%+25" means
	 *  the left edge is at 50% minus 150 pixels, the top
	 *  edge is at 10% plus 9 pixels, the right edge is at
	 *  50% plus 150 pixels, and the bottom edge is at 10%
	 *  plus 25 pixels.
	 * All four coordinates are required and can be
	 *  defined only in pixels, only in percents, or some
	 *  combination of both.
	 */

	// Check the input is only numeric
	const char* input = Value.c_str();
	CStr buffer = "";
	unsigned int coord = 0;
	float pixels[4] = {0, 0, 0, 0};
	float percents[4] = {0, 0, 0, 0};
	for (unsigned int i = 0; i < Value.length(); ++i)
	{
		switch (input[i])
		{
		case '.':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			buffer.push_back(input[i]);
			break;
		case '+':
			pixels[coord] += buffer.ToFloat();
			buffer = "+";
			break;
		case '-':
			pixels[coord] += buffer.ToFloat();
			buffer = "-";
			break;
		case '%':
			percents[coord] += buffer.ToFloat();
			buffer = "";
			break;
		case ' ':
			pixels[coord] += buffer.ToFloat();
			buffer = "";
			++coord;
			break;
		default:
			LOGERROR("CGUISize definition may only include numbers. Your input: '%s'", Value.c_str());
			return false;
		}
		if (coord > 3)
		{
			LOGERROR("Too many CGUISize parameters (4 max). Your input: '%s'", Value.c_str());
			return false;
		}
	}

	if (coord < 3)
	{
		LOGERROR("Too few CGUISize parameters (4 min). Your input: '%s'", Value.c_str());
		return false;
	}

	// Now that we're at the end of the string, flush the remaining buffer.
	pixels[coord] += buffer.ToFloat();

	// Now store the coords in the right place
	pixel.left =		pixels[0];
	pixel.top =			pixels[1];
	pixel.right =		pixels[2];
	pixel.bottom =		pixels[3];
	percent.left =		percents[0];
	percent.top =		percents[1];
	percent.right =		percents[2];
	percent.bottom =	percents[3];
	return true;
}

void CGUISize::ToJSVal(JSContext* cx, JS::MutableHandleValue ret) const
{
	JSAutoRequest rq(cx);
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	ret.setObjectOrNull(pScriptInterface->CreateCustomObject("GUISize"));

	if (!ret.isObject())
	{
		JS_ReportError(cx, "CGUISize value is not an Object");
		return;
	}

	JS::RootedObject obj(cx, &ret.toObject());
	if (!JS_InstanceOf(cx, obj, &JSI_GUISize::JSI_class, nullptr))
	{
		JS_ReportError(cx, "CGUISize value is not a CGUISize class instance");
		return;
	}

#define P(x, y, z)\
	if (!pScriptInterface->SetProperty(ret, #z, x.y)) \
	{ \
		JS_ReportError(cx, "Could not SetProperty '%s'", #z); \
		return; \
	}
	P(pixel, left, left);
	P(pixel, top, top);
	P(pixel, right, right);
	P(pixel, bottom, bottom);
	P(percent, left, rleft);
	P(percent, top, rtop);
	P(percent, right, rright);
	P(percent, bottom, rbottom);
#undef P
}

bool CGUISize::FromJSVal(JSContext* cx, JS::HandleValue v)
{
	JSAutoRequest rq(cx);
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;

	if (v.isString())
	{
		CStrW str;
		if (!ScriptInterface::FromJSVal(cx, v, str))
		{
			JS_ReportError(cx, "CGUISize could not read JS string");
			return false;
		}

		if (!FromString(str.ToUTF8()))
		{
			JS_ReportError(cx, "CGUISize could not parse JS string");
			return false;
		}
		return true;
	}

	if (!v.isObject())
	{
		JS_ReportError(cx, "CGUISize value is not an String, nor Object");
		return false;
	}

	JS::RootedObject obj(cx, &v.toObject());
	if (!JS_InstanceOf(cx, obj, &JSI_GUISize::JSI_class, nullptr))
	{
		JS_ReportError(cx, "CGUISize value is not a CGUISize class instance");
		return false;
	}

#define P(x, y, z) \
	if (!pScriptInterface->GetProperty(v, #z, x.y))\
	{\
		JS_ReportError(cx, "CGUISize could not get object property '%s'", #z);\
		return false;\
	}

	P(pixel, left, left);
	P(pixel, top, top);
	P(pixel, right, right);
	P(pixel, bottom, bottom);
	P(percent, left, rleft);
	P(percent, top, rtop);
	P(percent, right, rright);
	P(percent, bottom, rbottom);
#undef P

	return true;
}
