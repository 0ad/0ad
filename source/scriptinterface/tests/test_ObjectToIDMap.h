/* Copyright (C) 2016 Wildfire Games.
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
#include "scriptinterface/ScriptRuntime.h"
#include "scriptinterface/third_party/ObjectToIDMap.h"

class TestObjectToIDMap : public CxxTest::TestSuite
{
public:
	void test_movinggc()
	{
		ScriptInterface script("Test", "Test", g_ScriptRuntime);
		JSContext* cx = script.GetContext();
		JSAutoRequest rq(cx);

		JS::RootedObject obj(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
		ObjectIdCache<u32> map(g_ScriptRuntime);
		map.init();

		TS_ASSERT(map.add(cx, obj, 1));
		JSObject* plainObj = obj;
		// The map should contain the object we've just added
		TS_ASSERT(map.has(plainObj));

		JS_GC(g_ScriptRuntime->m_rt);

		// After a GC, the object should have been moved and plainObj should
		// not be valid anymore and not be found in the map anymore.
		// Obj should have an updated reference too, so it should still be found
		// in the map.
		//
		// NOTE: It's observed behaviour that a full GC always moves an object.
		// This might change in future SpiderMonkey versions. We only rely on
		// that behaviour for this test.
		//
		// TODO: It might be a good idea to test the behaviour when only a minor
		// GC runs, but there's no API for calling a minor GC yet.
		TS_ASSERT(plainObj != obj);
		TS_ASSERT(!map.has(plainObj));
		TS_ASSERT(map.has(obj));

		// Finding the ID associated with the object
		u32 ret(0);
		TS_ASSERT(map.find(obj, ret));
		TS_ASSERT_EQUALS(ret, 1);
	}
};
