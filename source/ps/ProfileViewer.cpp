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

/*
 * Implementation of profile display (containing only display routines,
 * the data model(s) are implemented elsewhere).
 */

#include "precompiled.h"

#include "ProfileViewer.h"

#include "graphics/Canvas2D.h"
#include "graphics/FontMetrics.h"
#include "graphics/TextRenderer.h"
#include "gui/GUIMatrix.h"
#include "lib/external_libraries/libsdl.h"
#include "maths/Size2D.h"
#include "maths/Vector2D.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/Hotkey.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "scriptinterface/Object.h"

#include <algorithm>
#include <fstream>
#include <ctime>

struct CProfileViewerInternals
{
	NONCOPYABLE(CProfileViewerInternals); // because of the ofstream
public:
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

	if (m->path.empty())
	{
		m->profileVisible = false;
		return;
	}

	PROFILE3_GPU("profile viewer");

	AbstractProfileTable* table = m->path[m->path.size() - 1];
	const std::vector<ProfileColumn>& columns = table->GetColumns();
	size_t numrows = table->GetNumberRows();

	CStrIntern font_name("mono-stroke-10");
	CFontMetrics font(font_name);
	int lineSpacing = font.GetLineSpacing();

	CCanvas2D canvas;

	// Render background.
	float estimateWidth = 50.0f;
	for (const ProfileColumn& column : columns)
		estimateWidth += static_cast<float>(column.width);

	float estimateHeight = 3 + static_cast<float>(numrows);
	if (m->path.size() > 1)
		estimateHeight += 2;
	estimateHeight *= lineSpacing;

	canvas.DrawRect(CRect(CSize2D(estimateWidth, estimateHeight)), CColor(0.0f, 0.0f, 0.0f, 0.5f));

	// Draw row backgrounds.
	for (size_t row = 0; row < numrows; ++row)
	{
		canvas.DrawRect(
			CRect(CVector2D(0.0f, lineSpacing * (2.0f + row) + 2.0f), CSize2D(estimateWidth, lineSpacing)),
			row % 2 ? CColor(1.0f, 1.0f, 1.0f, 0.1f): CColor(0.0f, 0.0f, 0.0f, 0.1f));
	}

	// Print table and column titles.
	CTextRenderer textRenderer;
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

		float rowColX = 0.0f;
		for (size_t col = 0; col < columns.size(); ++col)
		{
			CStrW text = table->GetCellText(row, col).FromUTF8();
			int w, h;
			font.CalculateStringSize(text.c_str(), w, h);

			float x = rowColX;
			if (col > 0) // right-align all but the first column
				x += columns[col].width - w;
			textRenderer.Put(x, 0.0f, text.c_str());

			rowColX += columns[col].width;
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

	canvas.DrawText(textRenderer);
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
	case SDL_HOTKEYPRESS:
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
	class WriteTable
	{
	public:
		WriteTable(std::ofstream& outputStream) : m_OutputStream(outputStream) {}
		WriteTable(const WriteTable& writeTable) = default;

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

			m_OutputStream << "\n\n" << table->GetTitle() << "\n";

			if (cols == 0) // avoid divide-by-zero
				return;

			for (size_t r = 0; r < data.size()/cols; ++r)
			{
				for (size_t c = 0; c < cols; ++c)
					m_OutputStream << (c ? " | " : "\n")
					  << data[r*cols + c].Pad(PS_TRIM_RIGHT, columnWidths[c]);

				// Add dividers under some rows. (Currently only the first, since
				// that contains the column headers.)
				if (r == 0)
					for (size_t c = 0; c < cols; ++c)
						m_OutputStream << (c ? "-|-" : "\n")
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
		std::ofstream& m_OutputStream;
		const WriteTable& operator=(const WriteTable&);
	};

	struct DumpTable
	{
		const ScriptInterface& m_ScriptInterface;
		JS::PersistentRooted<JS::Value> m_Root;
		DumpTable(const ScriptInterface& scriptInterface, JS::HandleValue root) :
			m_ScriptInterface(scriptInterface)
		{
			ScriptRequest rq(scriptInterface);
			m_Root.init(rq.cx, root);
		}

		// std::for_each requires a move constructor and the use of JS::PersistentRooted<T> apparently breaks a requirement for an
		// automatic move constructor
		DumpTable(DumpTable && original) :
			m_ScriptInterface(original.m_ScriptInterface)
		{
			ScriptRequest rq(m_ScriptInterface);
			m_Root.init(rq.cx, original.m_Root.get());
		}

		void operator() (AbstractProfileTable* table)
		{
			ScriptRequest rq(m_ScriptInterface);

			JS::RootedValue t(rq.cx);
			Script::CreateObject(
				rq,
				&t,
				"cols", DumpCols(table),
				"data", DumpRows(table));

			Script::SetProperty(rq, m_Root, table->GetTitle().c_str(), t);
		}

		std::vector<std::string> DumpCols(AbstractProfileTable* table)
		{
			std::vector<std::string> titles;

			const std::vector<ProfileColumn>& columns = table->GetColumns();

			for (size_t c = 0; c < columns.size(); ++c)
				titles.push_back(columns[c].title);

			return titles;
		}

		JS::Value DumpRows(AbstractProfileTable* table)
		{
			ScriptRequest rq(m_ScriptInterface);

			JS::RootedValue data(rq.cx);
			Script::CreateObject(rq, &data);

			const std::vector<ProfileColumn>& columns = table->GetColumns();

			for (size_t r = 0; r < table->GetNumberRows(); ++r)
			{
				JS::RootedValue row(rq.cx);
				Script::CreateArray(rq, &row);

				Script::SetProperty(rq, data, table->GetCellText(r, 0).c_str(), row);

				if (table->GetChild(r))
				{
					JS::RootedValue childRows(rq.cx, DumpRows(table->GetChild(r)));
					Script::SetPropertyInt(rq, row, 0, childRows);
				}

				for (size_t c = 1; c < columns.size(); ++c)
					Script::SetPropertyInt(rq, row, c, table->GetCellText(r, c));
			}

			return data;
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
			LOGERROR("Failed to open profile log file");
			return;
		}
		else
		{
			LOGMESSAGERENDER("Profiler snapshot saved to '%s'", path.string8());
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
