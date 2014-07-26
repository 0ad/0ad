/* Copyright (C) 2012 Wildfire Games.
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

/*
 * Implementation of profile display (containing only display routines,
 * the data model(s) are implemented elsewhere).
 */

#include "precompiled.h"

#include <ctime>
#include <algorithm>

#include "ProfileViewer.h"

#include "graphics/FontMetrics.h"
#include "gui/GUIutil.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextRenderer.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Hotkey.h"
#include "ps/Profile.h"
#include "lib/external_libraries/libsdl.h"
#include "renderer/Renderer.h"
#include "scriptinterface/ScriptInterface.h"

extern int g_xres, g_yres;

struct CProfileViewerInternals
{
	CProfileViewerInternals() {}

	/// Whether the profiling display is currently visible
	bool profileVisible;

	/// List of root tables
	std::vector<AbstractProfileTable*> rootTables;

	/// Path from a root table (path[0]) to the currently visible table (path[size-1])
	std::vector<AbstractProfileTable*> path;

	/// Helper functions
	void TableIsDeleted(AbstractProfileTable* table);
	void NavigateTree(int id);

	/// File for saved profile output (reset when the game is restarted)
	std::ofstream outputStream;

private:
	// Cannot be copied/assigned, because of the ofstream
	CProfileViewerInternals(const CProfileViewerInternals& rhs);
	const CProfileViewerInternals& operator=(const CProfileViewerInternals& rhs);
};


///////////////////////////////////////////////////////////////////////////////////////////////
// AbstractProfileTable implementation

AbstractProfileTable::~AbstractProfileTable()
{
	if (CProfileViewer::IsInitialised())
	{
		g_ProfileViewer.m->TableIsDeleted(this);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////
// CProfileViewer implementation


// AbstractProfileTable got deleted, make sure we have no dangling pointers
void CProfileViewerInternals::TableIsDeleted(AbstractProfileTable* table)
{
	for(int idx = (int)rootTables.size()-1; idx >= 0; --idx)
	{
		if (rootTables[idx] == table)
			rootTables.erase(rootTables.begin() + idx);
	}

	for(size_t idx = 0; idx < path.size(); ++idx)
	{
		if (path[idx] != table)
			continue;

		path.erase(path.begin() + idx, path.end());
		if (path.size() == 0)
			profileVisible = false;
	}
}


// Move into child tables or return to parent tables based on the given number
void CProfileViewerInternals::NavigateTree(int id)
{
	if (id == 0)
	{
		if (path.size() > 1)
			path.pop_back();
	}
	else
	{
		AbstractProfileTable* table = path[path.size() - 1];
		size_t numrows = table->GetNumberRows();

		for(size_t row = 0; row < numrows; ++row)
		{
			AbstractProfileTable* child = table->GetChild(row);

			if (!child)
				continue;

			--id;
			if (id == 0)
			{
				path.push_back(child);
				break;
			}
		}
	}
}


// Construction/Destruction
CProfileViewer::CProfileViewer()
{
	m = new CProfileViewerInternals;
	m->profileVisible = false;
}

CProfileViewer::~CProfileViewer()
{
	delete m;
}


// Render
void CProfileViewer::RenderProfile()
{
	if (!m->profileVisible)
		return;

	if (!m->path.size())
	{
		m->profileVisible = false;
		return;
	}

	PROFILE3_GPU("profile viewer");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	AbstractProfileTable* table = m->path[m->path.size() - 1];
	const std::vector<ProfileColumn>& columns = table->GetColumns();
	size_t numrows = table->GetNumberRows();

	CStrIntern font_name("mono-stroke-10");
	CFontMetrics font(font_name);
	int lineSpacing = font.GetLineSpacing();

	// Render background
	GLint estimate_height;
	GLint estimate_width;

	estimate_width = 50;
	for(size_t i = 0; i < columns.size(); ++i)
		estimate_width += (GLint)columns[i].width;

	estimate_height = 3 + (GLint)numrows;
	if (m->path.size() > 1)
		estimate_height += 2;
	estimate_height = lineSpacing*estimate_height;

	CShaderTechniquePtr solidTech = g_Renderer.GetShaderManager().LoadEffect(str_gui_solid);
	solidTech->BeginPass();
	CShaderProgramPtr solidShader = solidTech->GetShader();

	solidShader->Uniform(str_color, 0.0f, 0.0f, 0.0f, 0.5f);

	CMatrix3D transform = GetDefaultGuiMatrix();
	solidShader->Uniform(str_transform, transform);

	float backgroundVerts[] = {
		(float)estimate_width, 0.0f,
		0.0f, 0.0f,
		0.0f, (float)estimate_height,
		0.0f, (float)estimate_height,
		(float)estimate_width, (float)estimate_height,
		(float)estimate_width, 0.0f
	};
	solidShader->VertexPointer(2, GL_FLOAT, 0, backgroundVerts);
	solidShader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	transform.PostTranslate(22.0f, lineSpacing*3.0f, 0.0f);
	solidShader->Uniform(str_transform, transform);

	// Draw row backgrounds
	for (size_t row = 0; row < numrows; ++row)
	{
		if (row % 2)
			solidShader->Uniform(str_color, 1.0f, 1.0f, 1.0f, 0.1f);
		else
			solidShader->Uniform(str_color, 0.0f, 0.0f, 0.0f, 0.1f);

		float rowVerts[] = {
			-22.f, 2.f,
			estimate_width-22.f, 2.f,
			estimate_width-22.f, 2.f-lineSpacing,

			estimate_width-22.f, 2.f-lineSpacing,
			-22.f, 2.f-lineSpacing,
			-22.f, 2.f
		};
		solidShader->VertexPointer(2, GL_FLOAT, 0, rowVerts);
		solidShader->AssertPointersBound();
		glDrawArrays(GL_TRIANGLES, 0, 6);

		transform.PostTranslate(0.0f, lineSpacing, 0.0f);
		solidShader->Uniform(str_transform, transform);
	}

	solidTech->EndPass();

	// Print table and column titles

	CShaderTechniquePtr textTech = g_Renderer.GetShaderManager().LoadEffect(str_gui_text);
	textTech->BeginPass();

	CTextRenderer textRenderer(textTech->GetShader());
	textRenderer.Font(font_name);
	textRenderer.Color(1.0f, 1.0f, 1.0f);

	textRenderer.PrintfAt(2.0f, lineSpacing, L"%hs", table->GetTitle().c_str());

	textRenderer.Translate(22.0f, lineSpacing*2.0f, 0.0f);

	float colX = 0.0f;
	for (size_t col = 0; col < columns.size(); ++col)
	{
		CStrW text = columns[col].title.FromUTF8();
		int w, h;
		font.CalculateStringSize(text.c_str(), w, h);

		float x = colX;
		if (col > 0) // right-align all but the first column
			x += columns[col].width - w;
		textRenderer.Put(x, 0.0f, text.c_str());
		
		colX += columns[col].width;
	}

	textRenderer.Translate(0.0f, lineSpacing, 0.0f);

	// Print rows
	int currentExpandId = 1;

	for (size_t row = 0; row < numrows; ++row)
	{
		if (table->IsHighlightRow(row))
			textRenderer.Color(1.0f, 0.5f, 0.5f);
		else
			textRenderer.Color(1.0f, 1.0f, 1.0f);

		if (table->GetChild(row))
		{
			textRenderer.PrintfAt(-15.0f, 0.0f, L"%d", currentExpandId);
			currentExpandId++;
		}

		float colX = 0.0f;
		for (size_t col = 0; col < columns.size(); ++col)
		{
			CStrW text = table->GetCellText(row, col).FromUTF8();
			int w, h;
			font.CalculateStringSize(text.c_str(), w, h);

			float x = colX;
			if (col > 0) // right-align all but the first column
				x += columns[col].width - w;
			textRenderer.Put(x, 0.0f, text.c_str());

			colX += columns[col].width;
		}

		textRenderer.Translate(0.0f, lineSpacing, 0.0f);
	}

	textRenderer.Color(1.0f, 1.0f, 1.0f);

	if (m->path.size() > 1)
	{
		textRenderer.Translate(0.0f, lineSpacing, 0.0f);
		textRenderer.Put(-15.0f, 0.0f, L"0");
		textRenderer.Put(0.0f, 0.0f, L"back to parent");
	}

	textRenderer.Render();
	textTech->EndPass();

	glDisable(GL_BLEND);

	glEnable(GL_DEPTH_TEST);
}


// Handle input
InReaction CProfileViewer::Input(const SDL_Event_* ev)
{
	switch(ev->ev.type)
	{
	case SDL_KEYDOWN:
	{
		if (!m->profileVisible)
			break;

		int k = ev->ev.key.keysym.sym;
		if (k >= SDLK_0 && k <= SDLK_9)
		{
			m->NavigateTree(k - SDLK_0);
			return IN_HANDLED;
		}
		break;
	}
	case SDL_HOTKEYDOWN:
		std::string hotkey = static_cast<const char*>(ev->ev.user.data1);

		if( hotkey == "profile.toggle" )
		{
			if (!m->profileVisible)
			{
				if (m->rootTables.size())
				{
					m->profileVisible = true;
					m->path.push_back(m->rootTables[0]);
				}
			}
			else
			{
				size_t i;

				for(i = 0; i < m->rootTables.size(); ++i)
				{
					if (m->rootTables[i] == m->path[0])
						break;
				}
				i++;

				m->path.clear();
				if (i < m->rootTables.size())
				{
					m->path.push_back(m->rootTables[i]);
				}
				else
				{
					m->profileVisible = false;
				}
			}
			return( IN_HANDLED );
		}
		else if( hotkey == "profile.save" )
		{
			SaveToFile();
			return( IN_HANDLED );
		}
		break;
	}
	return( IN_PASS );
}

InReaction CProfileViewer::InputThunk(const SDL_Event_* ev)
{
	if (CProfileViewer::IsInitialised())
		return g_ProfileViewer.Input(ev);

	return IN_PASS;
}


// Add a table to the list of roots
void CProfileViewer::AddRootTable(AbstractProfileTable* table, bool front)
{
	if (front)
		m->rootTables.insert(m->rootTables.begin(), table);
	else
		m->rootTables.push_back(table);
}

namespace
{
	struct WriteTable
	{
		std::ofstream& f;
		WriteTable(std::ofstream& f) : f(f) {}

		void operator() (AbstractProfileTable* table)
		{
			std::vector<CStr> data; // 2d array of (rows+head)*columns elements

			const std::vector<ProfileColumn>& columns = table->GetColumns();

			// Add column headers to 'data'
			for (std::vector<ProfileColumn>::const_iterator col_it = columns.begin();
					col_it != columns.end(); ++col_it)
				data.push_back(col_it->title);

			// Recursively add all profile data to 'data'
			WriteRows(1, table, data);

			// Calculate the width of each column ( = the maximum width of
			// any value in that column)
			std::vector<size_t> columnWidths;
			size_t cols = columns.size();
			for (size_t c = 0; c < cols; ++c)
			{
				size_t max = 0;
				for (size_t i = c; i < data.size(); i += cols)
					max = std::max(max, data[i].length());
				columnWidths.push_back(max);
			}

			// Output data as a formatted table:

			f << "\n\n" << table->GetTitle() << "\n";

			if (cols == 0) // avoid divide-by-zero
				return;

			for (size_t r = 0; r < data.size()/cols; ++r)
			{
				for (size_t c = 0; c < cols; ++c)
					f << (c ? " | " : "\n")
					  << data[r*cols + c].Pad(PS_TRIM_RIGHT, columnWidths[c]);

				// Add dividers under some rows. (Currently only the first, since
				// that contains the column headers.)
				if (r == 0)
					for (size_t c = 0; c < cols; ++c)
						f << (c ? "-|-" : "\n")
						  << CStr::Repeat("-", columnWidths[c]);
			}
		}

		void WriteRows(int indent, AbstractProfileTable* table, std::vector<CStr>& data)
		{
			const std::vector<ProfileColumn>& columns = table->GetColumns();

			for (size_t r = 0; r < table->GetNumberRows(); ++r)
			{
				// Do pretty tree-structure indenting
				CStr indentation = CStr::Repeat("| ", indent-1);
				if (r+1 == table->GetNumberRows())
					indentation += "'-";
				else
					indentation += "|-";

				for (size_t c = 0; c < columns.size(); ++c)
					if (c == 0)
						data.push_back(indentation + table->GetCellText(r, c));
					else
						data.push_back(table->GetCellText(r, c));

				if (table->GetChild(r))
					WriteRows(indent+1, table->GetChild(r), data);
			}
		}

	private:
		const WriteTable& operator=(const WriteTable&);
	};

	struct DumpTable
	{
		ScriptInterface& scriptInterface;
		CScriptVal root;
		DumpTable(ScriptInterface& scriptInterface, JS::HandleValue root) :
			scriptInterface(scriptInterface), root(root)
		{
		}

		void operator() (AbstractProfileTable* table)
		{
			JSContext* cx = scriptInterface.GetContext();
			JSAutoRequest rq(cx);
			
			JS::RootedValue t(cx);
			scriptInterface.Eval(L"({})", &t);
			scriptInterface.SetProperty(t, "cols", DumpCols(table));
			scriptInterface.SetProperty(t, "data", DumpRows(table));
			
			JS::RootedValue tmpRoot(cx, root.get()); // TODO: Check if this temporary root can be removed after SpiderMonkey 31 upgrade 
			scriptInterface.SetProperty(tmpRoot, table->GetTitle().c_str(), t);
		}

		std::vector<std::string> DumpCols(AbstractProfileTable* table)
		{
			std::vector<std::string> titles;

			const std::vector<ProfileColumn>& columns = table->GetColumns();

			for (size_t c = 0; c < columns.size(); ++c)
				titles.push_back(columns[c].title);

			return titles;
		}

		CScriptVal DumpRows(AbstractProfileTable* table)
		{
			JSContext* cx = scriptInterface.GetContext();
			JSAutoRequest rq(cx);
			
			JS::RootedValue data(cx);
			scriptInterface.Eval("({})", &data);

			const std::vector<ProfileColumn>& columns = table->GetColumns();

			for (size_t r = 0; r < table->GetNumberRows(); ++r)
			{
				JS::RootedValue row(cx);
				scriptInterface.Eval("([])", &row);
				scriptInterface.SetProperty(data, table->GetCellText(r, 0).c_str(), row);

				if (table->GetChild(r))
					scriptInterface.SetPropertyInt(row, 0, DumpRows(table->GetChild(r)));

				for (size_t c = 1; c < columns.size(); ++c)
					scriptInterface.SetPropertyInt(row, c, table->GetCellText(r, c));
			}

			return data.get();
		}

	private:
		const DumpTable& operator=(const DumpTable&);
	};

	bool SortByName(AbstractProfileTable* a, AbstractProfileTable* b)
	{
		return (a->GetName() < b->GetName());
	}
}

void CProfileViewer::SaveToFile()
{
	// Open the file, if necessary. If this method is called several times,
	// the profile results will be appended to the previous ones from the same
	// run.
	if (! m->outputStream.is_open())
	{
		// Open the file. (It will be closed when the CProfileViewer
		// destructor is called.)
		OsPath path = psLogDir()/"profile.txt";
		m->outputStream.open(OsString(path).c_str(), std::ofstream::out | std::ofstream::trunc);

		if (m->outputStream.fail())
		{
			LOGERROR(L"Failed to open profile log file");
			return;
		}
		else
		{
			LOGMESSAGERENDER(L"Profiler snapshot saved to '%ls'", path.string().c_str());
		}
	}

	time_t t;
	time(&t);
	m->outputStream << "================================================================\n\n";
	m->outputStream << "PS profiler snapshot - " << asctime(localtime(&t));

	std::vector<AbstractProfileTable*> tables = m->rootTables;
	sort(tables.begin(), tables.end(), SortByName);
	for_each(tables.begin(), tables.end(), WriteTable(m->outputStream));

	m->outputStream << "\n\n================================================================\n";
	m->outputStream.flush();
}

CScriptVal CProfileViewer::SaveToJS(ScriptInterface& scriptInterface)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
		
	JS::RootedValue root(cx);
	scriptInterface.Eval("({})", &root);

	std::vector<AbstractProfileTable*> tables = m->rootTables;
	sort(tables.begin(), tables.end(), SortByName);
	for_each(tables.begin(), tables.end(), DumpTable(scriptInterface, root));

	return root.get();
}

void CProfileViewer::ShowTable(const CStr& table)
{
	m->path.clear();

	if (table.length() > 0)
	{
		for (size_t i = 0; i < m->rootTables.size(); ++i)
		{
			if (m->rootTables[i]->GetName() == table)
			{
				m->path.push_back(m->rootTables[i]);
				m->profileVisible = true;
				return;
			}
		}
	}

	// No matching table found, so don't display anything
	m->profileVisible = false;
}
