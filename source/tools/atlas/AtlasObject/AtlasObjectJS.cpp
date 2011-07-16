/* Copyright (C) 2011 Wildfire Games.
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

#include "AtlasObject.h"
#include "AtlasObjectImpl.h"

#include "../AtlasScript/ScriptInterface.h"

#include "wx/log.h"

#include <sstream>

static AtSmartPtr<AtNode> ConvertNode(JSContext* cx, jsval node);

AtObj AtlasObject::LoadFromJSON(JSContext* cx, const std::string& json)
{
	// Convert UTF8 to UTF16
	wxString jsonW(json.c_str(), wxConvUTF8);
	size_t json16len;
	wxCharBuffer json16 = wxMBConvUTF16().cWC2MB(jsonW.c_str(), jsonW.Length(), &json16len);

	jsval vp = JSVAL_NULL;
	JSONParser* parser = JS_BeginJSONParse(cx, &vp);
	if (!parser)
	{
		wxLogError(_T("ParseJSON failed to begin"));
		return AtObj();
	}

	if (!JS_ConsumeJSONText(cx, parser, reinterpret_cast<const jschar*>(json16.data()), (uint32)(json16len/2)))
	{
		wxLogError(_T("ParseJSON failed to consume"));
		return AtObj();
	}

	if (!JS_FinishJSONParse(cx, parser, JSVAL_NULL))
	{
		wxLogError(_T("ParseJSON failed to finish"));
		return AtObj();
	}

	AtObj obj;
	obj.p = ConvertNode(cx, vp);

	return obj;
}

// Convert from a jsval to an AtNode
static AtSmartPtr<AtNode> ConvertNode(JSContext* cx, jsval node)
{
	AtSmartPtr<AtNode> obj (new AtNode());

	// Non-objects get converted into strings
	if (!JSVAL_IS_OBJECT(node))
	{
		JSString* str = JS_ValueToString(cx, node);
		if (!str)
			return obj; // error
		size_t valueLen;
		const jschar* valueChars = JS_GetStringCharsAndLength(cx, str, &valueLen);
		if (!valueChars)
			return obj; // error
		wxString valueWx(reinterpret_cast<const char*>(valueChars), wxMBConvUTF16(), valueLen*2);

		obj->value = valueWx.c_str();

		// Annotate numbers/booleans specially, to allow round-tripping
		if (JSVAL_IS_NUMBER(node))
		{
			obj->children.insert(AtNode::child_pairtype(
				"@number", AtSmartPtr<AtNode>(new AtNode())
			));
		}
		else if (JSVAL_IS_BOOLEAN(node))
		{
			obj->children.insert(AtNode::child_pairtype(
				"@boolean", AtSmartPtr<AtNode>(new AtNode())
			));
		}

		return obj;
	}

	JSObject* it = JS_NewPropertyIterator(cx, JSVAL_TO_OBJECT(node));
	if (!it)
		return obj; // error

	while (true)
	{
		jsid idp;
		jsval val;
		if (! JS_NextProperty(cx, it, &idp) || ! JS_IdToValue(cx, idp, &val))
			return obj; // error
		if (val == JSVAL_VOID)
			break; // end of iteration
		if (! JSVAL_IS_STRING(val))
			continue; // ignore integer properties

		JSString* name = JSVAL_TO_STRING(val);
		size_t len;
		const jschar* chars = JS_GetStringCharsAndLength(cx, name, &len);
		wxString nameWx(reinterpret_cast<const char*>(chars), wxMBConvUTF16(), len*2);
		std::string nameStr(nameWx.ToUTF8().data());

		jsval vp;
		if (!JS_GetPropertyById(cx, JSVAL_TO_OBJECT(node), idp, &vp))
			return obj; // error

		// Unwrap arrays into a special format like <$name><item>$i0</item><item>...
		// (This assumes arrays aren't nested)
		if (JSVAL_IS_OBJECT(vp) && JS_IsArrayObject(cx, JSVAL_TO_OBJECT(vp)))
		{
			AtSmartPtr<AtNode> child(new AtNode());
			child->children.insert(AtNode::child_pairtype(
				"@array", AtSmartPtr<AtNode>(new AtNode())
			));

			jsuint arrayLength;
			if (!JS_GetArrayLength(cx, JSVAL_TO_OBJECT(vp), &arrayLength))
				return obj; // error

			for (jsuint i = 0; i < arrayLength; ++i)
			{
				jsval val;
				if (!JS_GetElement(cx, JSVAL_TO_OBJECT(vp), i, &val))
					return obj; // error

				child->children.insert(AtNode::child_pairtype(
					"item", ConvertNode(cx, val)
				));
			}

			obj->children.insert(AtNode::child_pairtype(
				nameStr, child
			));
		}
		else
		{
			obj->children.insert(AtNode::child_pairtype(
				nameStr, ConvertNode(cx, vp)
			));
		}
	}

	return obj;
}


jsval BuildJSVal(JSContext* cx, AtNode::Ptr p)
{
	if (!p)
		return JSVAL_VOID;

	// Special case for numbers/booleans to allow round-tripping
	if (p->children.count("@number"))
	{
		// Convert to double
		std::wstringstream str;
		str << p->value;
		double val = 0;
		str >> val;

		jsval rval;
		if (!JS_NewNumberValue(cx, val, &rval))
			return JSVAL_VOID; // error
		return rval;
	}
	else if (p->children.count("@boolean"))
	{
		bool val = false;
		if (p->value == L"true")
			val = true;

		return BOOLEAN_TO_JSVAL(val);
	}

	// If no children, then use the value string instead
	if (p->children.empty())
	{
		size_t val16len;
		wxCharBuffer val16 = wxMBConvUTF16().cWC2MB(p->value.c_str(), p->value.length(), &val16len);

		JSString* str = JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar*>(val16.data()), (uint32)(val16len/2));
		if (!str)
			return JSVAL_VOID; // error
		return STRING_TO_JSVAL(str);
	}

	if (p->children.find("@array") != p->children.end())
	{
		JSObject* obj = JS_NewArrayObject(cx, 0, NULL);
		if (!obj)
			return JSVAL_VOID; // error

		// Find the <item> children
		AtNode::child_maptype::const_iterator lower = p->children.lower_bound("item");
		AtNode::child_maptype::const_iterator upper = p->children.upper_bound("item");

		jsint idx = 0;
		for (AtNode::child_maptype::const_iterator it = lower; it != upper; ++it)
		{
			jsval val = BuildJSVal(cx, it->second);
			if (!JS_SetElement(cx, obj, idx, &val))
				return JSVAL_VOID; // error

			++idx;
		}

		return OBJECT_TO_JSVAL(obj);
	}
	else
	{
		JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
		if (!obj)
			return JSVAL_VOID; // error

		for (AtNode::child_maptype::const_iterator it = p->children.begin(); it != p->children.end(); ++it)
		{
			jsval val = BuildJSVal(cx, it->second);
			if (!JS_SetProperty(cx, obj, it->first.c_str(), &val))
				return JSVAL_VOID; // error
		}

		return OBJECT_TO_JSVAL(obj);
	}
}

struct Stringifier
{
	static JSBool callback(const jschar* buf, uint32 len, void* data)
	{
		wxString textWx(reinterpret_cast<const char*>(buf), wxMBConvUTF16(), len*2);
		std::string textStr(textWx.ToUTF8().data());

		static_cast<Stringifier*>(data)->stream << textStr;
		return JS_TRUE;
	}

	std::stringstream stream;
};

std::string AtlasObject::SaveToJSON(JSContext* cx, AtObj& obj)
{
	jsval root = BuildJSVal(cx, obj.p);

	Stringifier str;
	if (!JS_Stringify(cx, &root, NULL, JSVAL_VOID, &Stringifier::callback, &str))
	{
		wxLogError(_T("SaveToJSON failed"));
		return "";
	}

	return str.stream.str();
}
