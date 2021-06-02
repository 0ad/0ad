/* Copyright (C) 2021 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "lib/self_test.h"

#include "ps/CLogger.h"
#include "ps/Mod.h"
#include "scriptinterface/JSON.h"
#include "scriptinterface/ScriptInterface.h"

class TestMod : public CxxTest::TestSuite
{
	Mod m_Mods;
public:
	void test_version_check()
	{
		CStr eq = "=";
		CStr lt = "<";
		CStr gt = ">";
		CStr leq = "<=";
		CStr geq = ">=";

		CStr required = "0.0.24";// 0ad <= required
		CStr version = "0.0.24";// 0ad version

		// 0.0.24 = 0.0.24
		TS_ASSERT(m_Mods.CompareVersionStrings(version, eq, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, lt, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, gt, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, leq, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, geq, required));

		// 0.0.23 <= 0.0.24
		version = "0.0.23";
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, eq, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, lt, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, gt, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, leq, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, geq, required));

		// 0.0.25 >= 0.0.24
		version = "0.0.25";
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, eq, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, lt, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, gt, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, leq, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, geq, required));

		// 0.0.9 <= 0.1.0
		version = "0.0.9";
		required = "0.1.0";
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, eq, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, lt, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, gt, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, leq, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, geq, required));

		// 5.3 <= 5.3.0
		version = "5.3";
		required = "5.3.0";
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, eq, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, lt, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, gt, required));
		TS_ASSERT(m_Mods.CompareVersionStrings(version, leq, required));
		TS_ASSERT(!m_Mods.CompareVersionStrings(version, geq, required));
	}

	void test_compatible()
	{
		ScriptInterface script("Test", "Test", g_ScriptContext);

		ScriptRequest rq(script);
		JS::RootedObject obj(rq.cx, JS_NewPlainObject(rq.cx));

		m_Mods.m_AvailableMods = {
			Mod::ModData{ "public", "0ad", "0.0.25", {}, false, "" },
			Mod::ModData{ "wrong", "wrong", "0.0.1", { "0ad=0.0.24" }, false, "" },
			Mod::ModData{ "good", "good", "0.0.2", { "0ad=0.0.25" }, false, "" },
			Mod::ModData{ "good2", "good2", "0.0.4", { "0ad>=0.0.24" }, false, "" },
		};

		std::vector<CStr> mods;

		mods.clear();
		mods.push_back("public");
		TS_ASSERT(m_Mods.CheckForIncompatibleMods(mods).empty());

		mods.clear();
		mods.push_back("mod");
		mods.push_back("public");
		TS_ASSERT(m_Mods.CheckForIncompatibleMods(mods).empty());

		mods.clear();
		mods.push_back("public");
		mods.push_back("good");
		TS_ASSERT(m_Mods.CheckForIncompatibleMods(mods).empty());

		mods.clear();
		mods.push_back("public");
		mods.push_back("good2");
		TS_ASSERT(m_Mods.CheckForIncompatibleMods(mods).empty());

		mods.clear();
		mods.push_back("public");
		mods.push_back("wrong");
		TS_ASSERT(!m_Mods.CheckForIncompatibleMods(mods).empty());

		mods.clear();
		mods.push_back("public");
		mods.push_back("does_not_exist");
		TS_ASSERT(!m_Mods.CheckForIncompatibleMods(mods).empty());
	}

	void test_play_compatible()
	{
		Mod::ModData a1 = { "a", "a", "0.0.1", {}, false, "" };
		Mod::ModData a2 = { "a", "a", "0.0.2", {}, false, "" };
		Mod::ModData b = { "b", "b", "0.0.1", {}, false, "" };
		Mod::ModData c = { "c", "c", "0.0.1", {}, true, "" };

		using ModList = std::vector<const Mod::ModData*>;
		{
			ModList l1 = { &a1 };
			ModList l2 = { &a2 };
			TS_ASSERT(!Mod::AreModsPlayCompatible(l1, l2));
		}
		{
			ModList l1 = { &a1, &b };
			ModList l2 = { &a1, &b, &c };
			TS_ASSERT(Mod::AreModsPlayCompatible(l1, l2));
		}
		{
			ModList l1 = { &c, &b, &a1 };
			ModList l2 = { &b, &c, &a1 };
			TS_ASSERT(Mod::AreModsPlayCompatible(l1, l2));
		}
		{
			ModList l1 = { &b, &c, &a1 };
			ModList l2 = { &b, &c, &a2 };
			TS_ASSERT(!Mod::AreModsPlayCompatible(l1, l2));
		}
		{
			ModList l1 = { &c };
			ModList l2 = {};
			TS_ASSERT(Mod::AreModsPlayCompatible(l1, l2));
		}
		{
			ModList l1 = {};
			ModList l2 = { &b };
			TS_ASSERT(!Mod::AreModsPlayCompatible(l1, l2));
		}
	}
};
