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

#include "lib/self_test.h"

#include "gui/GUIManager.h"

#include "gui/CGUI.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Hotkey.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptRequest.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/StructuredClone.h"
#include "scriptinterface/Object.h"

#include <memory>

class TestGuiManager : public CxxTest::TestSuite
{
	std::unique_ptr<CConfigDB> configDB;
public:

	void setUp()
	{
		g_VFS = CreateVfs();
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir() / "mods" / "_test.gui" / "", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir() / "_testcache" / "", 0, VFS_MAX_PRIORITY));

		configDB = std::make_unique<CConfigDB>();

		CXeromyces::Startup();

		g_GUI = new CGUIManager();
	}

	void tearDown()
	{
		delete g_GUI;
		CXeromyces::Terminate();
		configDB.reset();
		g_VFS.reset();
		DeleteDirectory(DataDir()/"_testcache");
	}

	void test_EventObject()
	{
		// Load up a test page.
		const ScriptInterface& scriptInterface = *(g_GUI->GetScriptInterface());
		ScriptRequest rq(scriptInterface);
		JS::RootedValue val(rq.cx);
		Script::CreateObject(rq, &val);

		Script::StructuredClone data = Script::WriteStructuredClone(rq, JS::NullHandleValue);
		g_GUI->PushPage(L"event/page_event.xml", data, JS::UndefinedHandleValue);

		const ScriptInterface& pageScriptInterface = *(g_GUI->GetActiveGUI()->GetScriptInterface());
		ScriptRequest prq(pageScriptInterface);
		JS::RootedValue global(prq.cx, prq.globalValue());

		int called_value = 0;
		JS::RootedValue js_called_value(prq.cx);

		// Ticking will call the onTick handlers of all object. The second
		// onTick is configured to disable the onTick handlers of the first and
		// third and enable the fourth. So ticking once will only call the
		// first and second object. We don't want the fourth object to be
		// called, to avoid infinite additions of objects.
		g_GUI->TickObjects();
		Script::GetProperty(prq, global, "called1", &js_called_value);
		Script::FromJSVal(prq, js_called_value, called_value);
		TS_ASSERT_EQUALS(called_value, 1);

		Script::GetProperty(prq, global, "called2", &js_called_value);
		Script::FromJSVal(prq, js_called_value, called_value);
		TS_ASSERT_EQUALS(called_value, 1);

		Script::GetProperty(prq, global, "called3", &js_called_value);
		Script::FromJSVal(prq, js_called_value, called_value);
		TS_ASSERT_EQUALS(called_value, 0);

		Script::GetProperty(prq, global, "called4", &js_called_value);
		Script::FromJSVal(prq, js_called_value, called_value);
		TS_ASSERT_EQUALS(called_value, 0);

		// Ticking again will still call the second object, but also the fourth.
		g_GUI->TickObjects();
		Script::GetProperty(prq, global, "called1", &js_called_value);
		Script::FromJSVal(prq, js_called_value, called_value);
		TS_ASSERT_EQUALS(called_value, 1);

		Script::GetProperty(prq, global, "called2", &js_called_value);
		Script::FromJSVal(prq, js_called_value, called_value);
		TS_ASSERT_EQUALS(called_value, 2);

		Script::GetProperty(prq, global, "called3", &js_called_value);
		Script::FromJSVal(prq, js_called_value, called_value);
		TS_ASSERT_EQUALS(called_value, 0);

		Script::GetProperty(prq, global, "called4", &js_called_value);
		Script::FromJSVal(prq, js_called_value, called_value);
		TS_ASSERT_EQUALS(called_value, 1);
	}

	void test_hotkeysState()
	{
		// Load up a fake test hotkey when pressing 'a'.
		const char* test_hotkey_name = "hotkey.test";
		configDB->SetValueString(CFG_SYSTEM, test_hotkey_name, "A");
		LoadHotkeys(*configDB);

		// Load up a test page.
		const ScriptInterface& scriptInterface = *(g_GUI->GetScriptInterface());
		ScriptRequest rq(scriptInterface);
		JS::RootedValue val(rq.cx);
		Script::CreateObject(rq, &val);

		Script::StructuredClone data = Script::WriteStructuredClone(rq, JS::NullHandleValue);
		g_GUI->PushPage(L"hotkey/page_hotkey.xml", data, JS::UndefinedHandleValue);

		// Press 'a'.
		SDL_Event_ hotkeyNotification;
		hotkeyNotification.ev.type = SDL_KEYDOWN;
		hotkeyNotification.ev.key.keysym.scancode = SDL_SCANCODE_A;
		hotkeyNotification.ev.key.repeat = 0;

		// Init input and poll the event.
		InitInput();
		in_push_priority_event(&hotkeyNotification);
		SDL_Event_ ev;
		while (in_poll_event(&ev))
			in_dispatch_event(&ev);

		const ScriptInterface& pageScriptInterface = *(g_GUI->GetActiveGUI()->GetScriptInterface());
		ScriptRequest prq(pageScriptInterface);
		JS::RootedValue global(prq.cx, prq.globalValue());

		// Ensure that our hotkey state was synchronised with the event itself.
		bool hotkey_pressed_value = false;
		JS::RootedValue js_hotkey_pressed_value(prq.cx);

		Script::GetProperty(prq, global, "state_before", &js_hotkey_pressed_value);
		Script::FromJSVal(prq, js_hotkey_pressed_value, hotkey_pressed_value);
		TS_ASSERT_EQUALS(hotkey_pressed_value, true);

		hotkey_pressed_value = false;
		Script::GetProperty(prq, global, "state_after", &js_hotkey_pressed_value);
		Script::FromJSVal(prq, js_hotkey_pressed_value, hotkey_pressed_value);
		TS_ASSERT_EQUALS(hotkey_pressed_value, true);

		// We are listening to KeyDown events, so repeat shouldn't matter.
		hotkeyNotification.ev.key.repeat = 1;
		in_push_priority_event(&hotkeyNotification);
		while (in_poll_event(&ev))
			in_dispatch_event(&ev);

		hotkey_pressed_value = false;
		Script::GetProperty(prq, global, "state_before", &js_hotkey_pressed_value);
		Script::FromJSVal(prq, js_hotkey_pressed_value, hotkey_pressed_value);
		TS_ASSERT_EQUALS(hotkey_pressed_value, true);

		hotkey_pressed_value = false;
		Script::GetProperty(prq, global, "state_after", &js_hotkey_pressed_value);
		Script::FromJSVal(prq, js_hotkey_pressed_value, hotkey_pressed_value);
		TS_ASSERT_EQUALS(hotkey_pressed_value, true);

		hotkeyNotification.ev.type = SDL_KEYUP;
		in_push_priority_event(&hotkeyNotification);
		while (in_poll_event(&ev))
			in_dispatch_event(&ev);

		hotkey_pressed_value = true;
		Script::GetProperty(prq, global, "state_before", &js_hotkey_pressed_value);
		Script::FromJSVal(prq, js_hotkey_pressed_value, hotkey_pressed_value);
		TS_ASSERT_EQUALS(hotkey_pressed_value, false);

		hotkey_pressed_value = true;
		Script::GetProperty(prq, global, "state_after", &js_hotkey_pressed_value);
		Script::FromJSVal(prq, js_hotkey_pressed_value, hotkey_pressed_value);
		TS_ASSERT_EQUALS(hotkey_pressed_value, false);

		UnloadHotkeys();
	}

	void test_PageRegainedFocusEvent()
	{
		// Load up a test page.
		const ScriptInterface& scriptInterface = *(g_GUI->GetScriptInterface());
		ScriptRequest rq(scriptInterface);
		JS::RootedValue val(rq.cx);
		Script::CreateObject(rq, &val);

		Script::StructuredClone data = Script::WriteStructuredClone(rq, JS::NullHandleValue);
		g_GUI->PushPage(L"regainFocus/page_emptyPage.xml", data, JS::UndefinedHandleValue);

		const ScriptInterface& pageScriptInterface = *(g_GUI->GetActiveGUI()->GetScriptInterface());
		ScriptRequest prq(pageScriptInterface);
		JS::RootedValue global(prq.cx, prq.globalValue());

		g_GUI->PushPage(L"regainFocus/page_emptyPage.xml", data, JS::UndefinedHandleValue);
		g_GUI->PopPage(data);

		// This page instantly pushes an empty page with a callback that pops another page again.
		g_GUI->PushPage(L"regainFocus/page_pushWithPopOnInit.xml", data, JS::UndefinedHandleValue);

		// Pop the empty page and trigger the callback (effectively pops twice).
		g_GUI->PopPage(data);
	}
};
