/* Copyright (C) 2022 Wildfire Games.
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
#include "gui/CGUI.h"
#include "gui/CGUIText.h"
#include "gui/SettingTypes/CGUIString.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/ProfileViewer.h"
#include "ps/VideoMode.h"
#include "renderer/Renderer.h"

class TestCGUIText : public CxxTest::TestSuite
{
	CProfileViewer* m_Viewer = nullptr;
	CRenderer* m_Renderer = nullptr;
public:
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
		m_Viewer = new CProfileViewer;
		m_Renderer = new CRenderer;
	}

	void tearDown()
	{
		delete m_Renderer;
		delete m_Viewer;
		g_VideoMode.Shutdown();
		CConfigDB::Shutdown();
		CXeromyces::Terminate();
		g_VFS.reset();
		DeleteDirectory(DataDir()/"_testcache");
	}

	void test_empty()
	{
		CGUI gui(g_ScriptContext);
		CGUIText empty;
	}

	void test_wrapping()
	{
		CGUI gui(g_ScriptContext);

		static CStrW font = L"console";
		// Make sure this matches the value of the file.
		// TODO: load dynamically.
		static const float lineHeight = 12.f;
		static const float lineSpacing = 15.f;

		CGUIString string;
		CGUIText text;
		float width = 0.f;
		float renderedWidth = 0.f;
		float padding = 0.f;
		EAlign align = EAlign::LEFT;

		// Thing to note: the space before the newline should collapse in right-alignment.
		string.SetValue(L"Some long text that will wrap-around. \n New line.");
		text = CGUIText(gui, string, font, width, padding, align, nullptr);

		// Width 0 means no wrapping, so we should be getting one render call & one line.
		// TODO: is it wanted that \n doesn't wrap in that case?
		// We have 11 calls: the 9 words (wrap-around is split in two), the space after the newline, and the newline itself.
		TS_ASSERT_EQUALS(text.GetTextCalls().size(), 11);
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight);

		width = 100.f;
		padding = 2.0f;
		align = EAlign::LEFT;
		text = CGUIText(gui, string, font, width, padding, align, nullptr);
		renderedWidth = text.GetSize().Width;
		// We have 10 calls: the 9 words (wrap-around is split in two), the space after the newline.
		TS_ASSERT_EQUALS(text.GetTextCalls().size(), 10);
		TS_ASSERT_LESS_THAN(text.GetSize().Width, width);
		TS_ASSERT_EQUALS(text.GetSize().Height, padding * 2 + lineHeight + lineSpacing * 4);

		align = EAlign::RIGHT;
		text = CGUIText(gui, string, font, width, padding, align, nullptr);
		TS_ASSERT_EQUALS(text.GetTextCalls().size(), 10);
		TS_ASSERT_EQUALS(text.GetSize().Width, renderedWidth); // Should be the same width as the left-case.
		TS_ASSERT_EQUALS(text.GetSize().Height, padding * 2 + lineHeight + lineSpacing * 4);

		width = 400.f;
		padding = 3.0f;
		align = EAlign::LEFT;
		text = CGUIText(gui, string, font, width, padding, align, nullptr);
		TS_ASSERT_EQUALS(text.GetTextCalls().size(), 10);
		TS_ASSERT_LESS_THAN(text.GetSize().Width, width);
		TS_ASSERT_EQUALS(text.GetSize().Height, padding * 2 + lineHeight + lineSpacing);

		width = 400.f;
		padding = 5.0f;
		align = EAlign::CENTER;
		text = CGUIText(gui, string, font, width, padding, align, nullptr);
		renderedWidth = text.GetSize().Width;
		TS_ASSERT_LESS_THAN(text.GetSize().Width, width);
		TS_ASSERT_EQUALS(text.GetSize().Height, padding * 2 + lineHeight + lineSpacing);

		align = EAlign::RIGHT;
		text = CGUIText(gui, string, font, width, padding, align, nullptr);
		TS_ASSERT_EQUALS(text.GetSize().Width, renderedWidth); // Should be the same width as the center-case.
		TS_ASSERT_EQUALS(text.GetSize().Height, padding * 2 + lineHeight + lineSpacing);

		align = EAlign::LEFT;
		text = CGUIText(gui, string, font, width, padding, align, nullptr);
		TS_ASSERT_EQUALS(text.GetSize().Width, renderedWidth); // Should be the same width as the center-case.
		TS_ASSERT_EQUALS(text.GetSize().Height, padding * 2 + lineHeight + lineSpacing);

		width = 400.f;
		padding = 100.0f;
		align = EAlign::LEFT;
		text = CGUIText(gui, string, font, width, padding, align, nullptr);
		renderedWidth = text.GetSize().Width;
		TS_ASSERT_LESS_THAN(text.GetSize().Width, width);
		TS_ASSERT_EQUALS(text.GetSize().Height, padding * 2 + lineHeight + lineSpacing * 2);

		align = EAlign::RIGHT;
		text = CGUIText(gui, string, font, width, padding, align, nullptr);
		TS_ASSERT_EQUALS(text.GetSize().Width, renderedWidth); // Should be the same width as the left-case.
		TS_ASSERT_EQUALS(text.GetSize().Height, padding * 2 + lineHeight + lineSpacing * 2);
	}

	void test_overflow()
	{
		CGUI gui(g_ScriptContext);

		static CStrW font = L"console";
		// Make sure this matches the value of the file.
		// TODO: load dynamically.
		static const float lineHeight = 12.f;
		static const float lineSpacing = 15.f;

		float renderedWidth = 0.f;
		const float width = 200.f;
		const float padding = 20.f;

		CGUIString string;
		CGUIText text;
		string.SetValue(L"wordthatisverylonganddefinitelywontfitinaline and other words");
		text = CGUIText(gui, string, font, width, padding, EAlign::LEFT, nullptr);
		renderedWidth = text.GetSize().Width;
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + lineSpacing * 1 + padding * 2);
		text = CGUIText(gui, string, font, width, padding, EAlign::CENTER, nullptr);
		TS_ASSERT_EQUALS(renderedWidth, text.GetSize().Width);
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + lineSpacing * 1 + padding * 2);
		text = CGUIText(gui, string, font, width, padding, EAlign::RIGHT, nullptr);
		TS_ASSERT_EQUALS(renderedWidth, text.GetSize().Width);
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + lineSpacing * 1 + padding * 2);

		string.SetValue(L"other words and wordthatisverylonganddefinitelywontfitinaline");
		text = CGUIText(gui, string, font, width, padding, EAlign::LEFT, nullptr);
		renderedWidth = text.GetSize().Width;
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + lineSpacing * 1 + padding * 2);
		text = CGUIText(gui, string, font, width, padding, EAlign::CENTER, nullptr);
		TS_ASSERT_EQUALS(renderedWidth, text.GetSize().Width);
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + lineSpacing * 1 + padding * 2);
		text = CGUIText(gui, string, font, width, padding, EAlign::RIGHT, nullptr);
		TS_ASSERT_EQUALS(renderedWidth, text.GetSize().Width);
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + lineSpacing * 1 + padding * 2);

		string.SetValue(L"wordthatisverylonganddefinitelywontfitinaline");
		text = CGUIText(gui, string, font, width, padding, EAlign::LEFT, nullptr);
		renderedWidth = text.GetSize().Width;
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + padding * 2);
		text = CGUIText(gui, string, font, width, padding, EAlign::CENTER, nullptr);
		TS_ASSERT_EQUALS(renderedWidth, text.GetSize().Width);
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + padding * 2);
		text = CGUIText(gui, string, font, width, padding, EAlign::RIGHT, nullptr);
		TS_ASSERT_EQUALS(renderedWidth, text.GetSize().Width);
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + padding * 2);
	}

	void test_regression_rP26522()
	{
		TS_ASSERT_OK(g_VFS->Mount(L"", DataDir() / "mods" / "mod" / "", VFS_MOUNT_MUST_EXIST));

		CGUI gui(g_ScriptContext);

		static CStrW font = L"sans-bold-13";
		CGUIString string;
		CGUIText text;

		// rP26522 introduced a bug that triggered in rare cases with word-wrapping.
		string.SetValue(L"90–120 min");
		text = CGUIText(gui, string, L"sans-bold-13", 53, 8.f, EAlign::LEFT, nullptr);

		TS_ASSERT_EQUALS(text.GetTextCalls().size(), 2);
		TS_ASSERT_EQUALS(text.GetSize().Height, 14 + 9 + 8 * 2);
	}

	void test_multiple_blank_spaces()
	{
		CGUI gui(g_ScriptContext);

		static CStrW font = L"console";
		// Make sure this matches the value of the file.
		// TODO: load dynamically.
		static const float lineHeight = 12.f;
		static const float lineSpacing = 15.f;

		CGUIString string;
		CGUIText text;
		float width = 100.f;
		float renderedWidth = 0.f;
		float padding = 0.f;
		EAlign align = EAlign::LEFT;

		string.SetValue(L"   word    another    \n    spaces   \n   \n  word   ");
		text = CGUIText(gui, string, font, width, padding, align, nullptr);

		// Blank spaces are treated as a word.
		TS_ASSERT_EQUALS(text.GetTextCalls().size(), 26);
		TS_ASSERT_EQUALS(text.GetSize().Height, lineHeight + lineSpacing * 4);
		TS_ASSERT_EQUALS(text.GetSize().Width, 89.f);
		renderedWidth = text.GetSize().Width;

		align = EAlign::RIGHT;
		text = CGUIText(gui, string, font, width, padding, align, nullptr);
		TS_ASSERT_EQUALS(text.GetSize().Width, renderedWidth);
	}
};
