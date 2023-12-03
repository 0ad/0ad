/* Copyright (C) 2023 Wildfire Games.
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

#include "graphics/FontManager.h"
#include "graphics/FontMetrics.h"
#include "gui/CGUI.h"
#include "gui/CGUISetting.h"
#include "gui/CGUIText.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/SettingTypes/CGUIString.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/ProfileViewer.h"
#include "ps/VideoMode.h"
#include "renderer/Renderer.h"
#include "scriptinterface/ScriptInterface.h"

#include <memory>
#include <type_traits>

class TestGUISetting : public CxxTest::TestSuite
{
	std::unique_ptr<CProfileViewer> m_Viewer;
	std::unique_ptr<CRenderer> m_Renderer;

public:
	class TestGUIObject : public IGUIObject
	{
	public:
		TestGUIObject(CGUI& gui) : IGUIObject(gui) {}

		void Draw(CCanvas2D&) {}
	};

	void setUp()
	{
		g_VFS = CreateVfs();
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir() / "mods" / "_test.minimal" / "", VFS_MOUNT_MUST_EXIST));
		TS_ASSERT_OK(g_VFS->Mount(L"cache", DataDir() / "_testcache" / "", 0, VFS_MAX_PRIORITY));

		CXeromyces::Startup();

		// The renderer spews messages.
		TestLogger logger;

		// We need to initialise the renderer to initialise the font manager.
		// TODO: decouple this.
		CConfigDB::Initialise();
		CConfigDB::Instance()->SetValueString(CFG_SYSTEM, "rendererbackend", "dummy");
		g_VideoMode.InitNonSDL();
		g_VideoMode.CreateBackendDevice(false);
		m_Viewer = std::make_unique<CProfileViewer>();
		m_Renderer = std::make_unique<CRenderer>(g_VideoMode.GetBackendDevice());
	}

	void tearDown()
	{
		m_Renderer.reset();
		m_Viewer.reset();
		g_VideoMode.Shutdown();
		CConfigDB::Shutdown();
		CXeromyces::Terminate();
		g_VFS.reset();
		DeleteDirectory(DataDir() / "_testcache");
	}

	void test_movability()
	{
		CGUI gui(g_ScriptContext);
		TestGUIObject object(gui);

		static_assert(std::is_move_constructible_v<CGUISimpleSetting<CStr>>);
		static_assert(!std::is_move_assignable_v<CGUISimpleSetting<CStr>>);

		CGUISimpleSetting<CStr> settingA(&object, "A");
		TS_ASSERT(settingA->empty());
		TS_ASSERT(object.SettingExists("A"));
		object.SetSettingFromString("A", L"ValueA", false);
		TS_ASSERT_EQUALS(*settingA, "ValueA");

		CGUISimpleSetting<CStr> settingB(std::move(settingA));
		TS_ASSERT(object.SettingExists("A"));
		object.SetSettingFromString("A", L"ValueB", false);
		TS_ASSERT_EQUALS(*settingB, "ValueB");
	}
};
