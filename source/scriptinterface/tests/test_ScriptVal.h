/* Copyright (C) 2010 Wildfire Games.
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

#include "lib/self_test.h"

#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptVal.h"

#include "jsapi.h"

class TestScriptVal : public CxxTest::TestSuite
{
public:
	void test_rooting()
	{
		ScriptInterface script("Test", "Test", ScriptInterface::CreateRuntime());
		JSContext* cx = script.GetContext();
		JSAutoRequest rq(cx);

		JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
		TS_ASSERT(obj);

		CScriptValRooted root(cx, OBJECT_TO_JSVAL(obj));

		JS_GC(script.GetJSRuntime());

		jsval val = INT_TO_JSVAL(123);
		TS_ASSERT(JS_SetProperty(cx, obj, "test", &val));

		JS_GC(script.GetJSRuntime());

		jsval rval;
		TS_ASSERT(JS_GetProperty(cx, obj, "test", &rval));
		TS_ASSERT(JSVAL_IS_INT(rval));
		TS_ASSERT(JSVAL_TO_INT(rval) == 123);
	}
};
