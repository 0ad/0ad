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

#include "lib/external_libraries/libsdl.h"
#include "ps/Hotkey.h"
#include "ps/ConfigDB.h"
#include "ps/Globals.h"
#include "ps/Filesystem.h"


class TestHotkey : public CxxTest::TestSuite
{
	CConfigDB* configDB;

private:

	void fakeInput(const char* key, bool keyDown)
	{
		SDL_Event_ ev;
		ev.ev.type = keyDown ? SDL_KEYDOWN : SDL_KEYUP;
		ev.ev.key.repeat = 0;
		ev.ev.key.keysym.scancode = SDL_GetScancodeFromName(key);
		GlobalsInputHandler(&ev);
		HotkeyInputHandler(&ev);
		while(in_poll_priority_event(&ev))
			HotkeyStateChange(&ev);
	}

public:
	void setUp()
	{
		g_VFS = CreateVfs();
		TS_ASSERT_OK(g_VFS->Mount(L"config", DataDir() / "_testconfig" / ""));
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir() / "_testcache" / ""));

		configDB = new CConfigDB;

		g_scancodes = {};
	}

	void tearDown()
	{
		delete configDB;
		g_VFS.reset();
		DeleteDirectory(DataDir()/"_testcache");
		DeleteDirectory(DataDir()/"_testconfig");
	}

	void test_Hotkeys()
	{
		configDB->SetValueString(CFG_SYSTEM, "hotkey.A", "A");
		configDB->SetValueString(CFG_SYSTEM, "hotkey.AB", "A+B");
		configDB->SetValueString(CFG_SYSTEM, "hotkey.ABC", "A+B+C");
		configDB->SetValueList(CFG_SYSTEM, "hotkey.D", { "D", "D+E" });
		configDB->WriteFile(CFG_SYSTEM, "config/conf.cfg");
		configDB->Reload(CFG_SYSTEM);

		UnloadHotkeys();
		LoadHotkeys(*configDB);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		/**
		 * Simple check.
		 */
		fakeInput("A", true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		fakeInput("A", false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		/**
		 * Hotkey combinations:
		 *  - The most precise match only is selected
		 *  - Order does not matter.
		 */
		fakeInput("A", true);
		fakeInput("B", true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		fakeInput("A", false);
		fakeInput("B", false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);

		fakeInput("B", true);
		fakeInput("A", true);
		// Activating the more precise hotkey AB untriggers "A"
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		fakeInput("A", false);
		fakeInput("B", false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		fakeInput("A", true);
		fakeInput("B", true);
		fakeInput("B", false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		fakeInput("A", false);
		fakeInput("D", true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);

		fakeInput("E", true);
		// Changing from one hotkey to another more specific combination of the same hotkey keeps it active
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		fakeInput("E", false);
		// Likewise going the other way.
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
	}

	void test_quirk()
	{
		configDB->SetValueString(CFG_SYSTEM, "hotkey.A", "A");
		configDB->SetValueString(CFG_SYSTEM, "hotkey.AB", "A+B");
		configDB->SetValueString(CFG_SYSTEM, "hotkey.ABC", "A+B+C");
		configDB->SetValueList(CFG_SYSTEM, "hotkey.D", { "D", "D+E" });
		configDB->WriteFile(CFG_SYSTEM, "config/conf.cfg");
		configDB->Reload(CFG_SYSTEM);

		UnloadHotkeys();
		LoadHotkeys(*configDB);

		/**
		 * Quirk of the implementation: hotkeys are allowed to fire with too many keys.
		 * Further, hotkeys of the same specificity (i.e. same # of required keys)
		 * are allowed to fire at the same time if they don't conflict.
		 * This is required so that e.g. up+left scrolls both up and left at the same time.
		 */
		fakeInput("A", true);
		fakeInput("D", true);
		// A+D isn't a hotkey; both A and D are active.
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);

		fakeInput("C", true);
		// A+D+C likewise.
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);

		fakeInput("B", true);
		// Here D is inactivated because it's lower-specificity than A+B+C (with D being ignored).
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		fakeInput("A", false);
		fakeInput("B", false);
		fakeInput("C", false);
		fakeInput("D", false);

		fakeInput("B", true);
		fakeInput("D", true);
		fakeInput("A", true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		UnloadHotkeys();
	}
};
