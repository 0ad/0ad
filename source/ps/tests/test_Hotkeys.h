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
	std::unique_ptr<CConfigDB> configDB;
	// Stores whether one of these was sent in the last fakeInput call.
	bool hotkeyPress = false;
	bool hotkeyUp = false;

private:

	void fakeInput(const char* key, bool keyDown)
	{
		SDL_Event_ ev;
		ev.ev.type = keyDown ? SDL_KEYDOWN : SDL_KEYUP;
		ev.ev.key.repeat = 0;
		ev.ev.key.keysym.scancode = SDL_GetScancodeFromName(key);
		GlobalsInputHandler(&ev);
		HotkeyInputPrepHandler(&ev);
		HotkeyInputActualHandler(&ev);
		hotkeyPress = false;
		hotkeyUp = false;
		while(in_poll_priority_event(&ev))
		{
			hotkeyUp |= ev.ev.type == SDL_HOTKEYUP;
			hotkeyPress |= ev.ev.type == SDL_HOTKEYPRESS;
			HotkeyStateChange(&ev);
		}
	}

public:
	void setUp()
	{
		g_VFS = CreateVfs();
		TS_ASSERT_OK(g_VFS->Mount(L"config", DataDir() / "_testconfig" / ""));
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir() / "_testcache" / ""));

		configDB = std::make_unique<CConfigDB>();

		g_scancodes = {};
	}

	void tearDown()
	{
		configDB.reset();
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
		TS_ASSERT_EQUALS(hotkeyPress, true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		fakeInput("A", false);
		TS_ASSERT_EQUALS(hotkeyUp, true);
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
		// HotkeyUp is true - A is released.
		TS_ASSERT_EQUALS(hotkeyUp, true);
		TS_ASSERT_EQUALS(hotkeyPress, true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		fakeInput("B", false);
		// A is silently retriggered - no Press
		TS_ASSERT_EQUALS(hotkeyPress, false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), true);

		fakeInput("A", false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);

		fakeInput("B", true);
		fakeInput("A", true);
		TS_ASSERT_EQUALS(hotkeyUp, false);
		TS_ASSERT_EQUALS(hotkeyPress, true);
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
		TS_ASSERT_EQUALS(hotkeyUp, true);
		// The "A" is retriggered silently.
		TS_ASSERT_EQUALS(hotkeyPress, false);
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
	}

	void test_double_combination()
	{
		configDB->SetValueString(CFG_SYSTEM, "hotkey.AB", "A+B");
		configDB->SetValueList(CFG_SYSTEM, "hotkey.D", { "D", "E" });
		configDB->WriteFile(CFG_SYSTEM, "config/conf.cfg");
		configDB->Reload(CFG_SYSTEM);

		UnloadHotkeys();
		LoadHotkeys(*configDB);

		// Bit of a special case > Both D and E trigger the same hotkey.
		// In that case, any key change that still gets the hotkey triggered
		// will re-trigger a "press", and on the final release, the "Up" is sent.
		fakeInput("D", true);
		TS_ASSERT_EQUALS(hotkeyUp, false);
		TS_ASSERT_EQUALS(hotkeyPress, true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		fakeInput("E", true);
		TS_ASSERT_EQUALS(hotkeyUp, false);
		TS_ASSERT_EQUALS(hotkeyPress, true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		fakeInput("D", false);
		TS_ASSERT_EQUALS(hotkeyUp, false);
		TS_ASSERT_EQUALS(hotkeyPress, true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		fakeInput("E", false);
		TS_ASSERT_EQUALS(hotkeyUp, true);
		TS_ASSERT_EQUALS(hotkeyPress, false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);

		// Check that silent triggering works even in that case.
		fakeInput("D", true);
		fakeInput("E", true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		fakeInput("A", true);
		fakeInput("B", true);
		TS_ASSERT_EQUALS(hotkeyUp, true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);
		fakeInput("B", false);
		TS_ASSERT_EQUALS(hotkeyPress, false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		fakeInput("E", false);
		TS_ASSERT_EQUALS(hotkeyUp, false);
		TS_ASSERT_EQUALS(hotkeyPress, false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		// Note: as a consequence of the special case here - repressing E won't trigger a "press".
		fakeInput("E", true);
		TS_ASSERT_EQUALS(hotkeyUp, false);
		TS_ASSERT_EQUALS(hotkeyPress, false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		fakeInput("E", false);
		TS_ASSERT_EQUALS(hotkeyUp, false);
		TS_ASSERT_EQUALS(hotkeyPress, false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		fakeInput("D", false);
		TS_ASSERT_EQUALS(hotkeyUp, false);
		TS_ASSERT_EQUALS(hotkeyPress, false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);
	}

	void test_quirk()
	{
		configDB->SetValueString(CFG_SYSTEM, "hotkey.A", "A");
		configDB->SetValueString(CFG_SYSTEM, "hotkey.AB", "A+B");
		configDB->SetValueString(CFG_SYSTEM, "hotkey.ABC", "A+B+C");
		configDB->SetValueList(CFG_SYSTEM, "hotkey.D", { "D", "D+E" });
		configDB->SetValueString(CFG_SYSTEM, "hotkey.E", "E+C");
		configDB->WriteFile(CFG_SYSTEM, "config/conf.cfg");
		configDB->Reload(CFG_SYSTEM);

		UnloadHotkeys();
		LoadHotkeys(*configDB);

		/**
		 * Quirk of the implementation: hotkeys are allowed to fire with too many keys.
		 * Further, hotkeys with the same # of keys are allowed to trigger at the same time.
		 * This is required to make e.g. 'up' and 'left' scroll up-left when both are active.
		 * It would be nice to extend this to 'non-conflicting hotkeys', but that's quickly far more complex.
		 */
		fakeInput("A", true);
		fakeInput("D", true);
		// A+D isn't a hotkey; both A and D are active.
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("E"), false);

		fakeInput("C", true);
		// A+D+C likewise.
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("E"), false);

		fakeInput("B", true);
		// A+B+C is a hotkey, more specific than A and D - both deactivated.
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("E"), false);

		fakeInput("E", true);
		// D+E is still less specific than A+B+C - nothing changes.
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("E"), false);

		fakeInput("B", false);
		// E & D activated - D+E and E+C have the same specificity.
		// The triggering is silent as it's from a key release.
		TS_ASSERT_EQUALS(hotkeyUp, true);
		TS_ASSERT_EQUALS(hotkeyPress, false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("E"), true);

		fakeInput("E", false);
		// A and D again.
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("E"), false);

		fakeInput("A", false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), true);
		TS_ASSERT_EQUALS(HotkeyIsPressed("E"), false);

		fakeInput("D", false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("A"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("AB"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("ABC"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("D"), false);
		TS_ASSERT_EQUALS(HotkeyIsPressed("E"), false);
		UnloadHotkeys();
	}
};
