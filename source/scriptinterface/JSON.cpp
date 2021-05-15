/* Copyright (C) 2021 Wildfire Games.
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

#include "JSON.h"

#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "ps/utf16string.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptExceptions.h"
#include "scriptinterface/ScriptRequest.h"
#include "scriptinterface/ScriptTypes.h"

// Ignore warnings in SM headers.
#if GCC_VERSION || CLANG_VERSION
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#elif MSC_VERSION
# pragma warning(push, 1)
#endif

#include "js/JSON.h"

#if GCC_VERSION || CLANG_VERSION
# pragma GCC diagnostic pop
#elif MSC_VERSION
# pragma warning(pop)
#endif

bool Script::ParseJSON(const ScriptRequest& rq, const std::string& string_utf8, JS::MutableHandleValue out)
{
	std::wstring attrsW = wstring_from_utf8(string_utf8);
	utf16string string(attrsW.begin(), attrsW.end());
	if (JS_ParseJSON(rq.cx, reinterpret_cast<const char16_t*>(string.c_str()), (u32)string.size(), out))
		return true;

	ScriptException::CatchPending(rq);
	return false;
}

void Script::ReadJSONFile(const ScriptRequest& rq, const VfsPath& path, JS::MutableHandleValue out)
{
	if (!VfsFileExists(path))
	{
		LOGERROR("File '%s' does not exist", path.string8());
		return;
	}

	CVFSFile file;

	PSRETURN ret = file.Load(g_VFS, path);

	if (ret != PSRETURN_OK)
	{
		LOGERROR("Failed to load file '%s': %s", path.string8(), GetErrorString(ret));
		return;
	}

	std::string content(file.DecodeUTF8()); // assume it's UTF-8

	if (!ParseJSON(rq, content, out))
		LOGERROR("Failed to parse '%s'", path.string8());
}

namespace
{
struct Stringifier
{
	static bool callback(const char16_t* buf, u32 len, void* data)
	{
		utf16string str(buf, buf+len);
		std::wstring strw(str.begin(), str.end());

		Status err; // ignore Unicode errors
		static_cast<Stringifier*>(data)->stream << utf8_from_wstring(strw, &err);
		return true;
	}

	std::stringstream stream;
};
} // anonymous namespace

// JS_Stringify takes a mutable object because ToJSON may arbitrarily mutate the value.
// TODO: it'd be nice to have a non-mutable variant.
std::string Script::StringifyJSON(const ScriptRequest& rq, JS::MutableHandleValue obj, bool indent)
{
	Stringifier str;
	JS::RootedValue indentVal(rq.cx, indent ? JS::Int32Value(2) : JS::UndefinedValue());
	if (!JS_Stringify(rq.cx, obj, nullptr, indentVal, &Stringifier::callback, &str))
	{
		ScriptException::CatchPending(rq);
		return std::string();
	}

	return str.stream.str();
}


std::string Script::ToString(const ScriptRequest& rq, JS::MutableHandleValue obj, bool pretty)
{
	if (obj.isUndefined())
		return "(void 0)";

	// Try to stringify as JSON if possible
	// (TODO: this is maybe a bad idea since it'll drop 'undefined' values silently)
	if (pretty)
	{
		Stringifier str;
		JS::RootedValue indentVal(rq.cx, JS::Int32Value(2));

		if (JS_Stringify(rq.cx, obj, nullptr, indentVal, &Stringifier::callback, &str))
			return str.stream.str();

		// Drop exceptions raised by cyclic values before trying something else
		JS_ClearPendingException(rq.cx);
	}

	// Caller didn't want pretty output, or JSON conversion failed (e.g. due to cycles),
	// so fall back to obj.toSource()

	std::wstring source = L"(error)";
	ScriptFunction::Call(rq, obj, "toSource", source);
	return utf8_from_wstring(source);
}
