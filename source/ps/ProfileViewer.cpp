/* Copyright (C) 2009 Wildfire Games.
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

#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Hotkey.h"
#include "ps/Profile.h"
#include "lib/wchar.h"
#include "lib/external_libraries/sdl.h"
#include "lib/res/graphics/unifont.h"
#include "renderer/Renderer.h"

#define LOG_CATEGORY L"profiler"
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	AbstractProfileTable* table = m->path[m->path.size() - 1];
	const std::vector<ProfileColumn>& columns = table->GetColumns();
	size_t numrows = table->GetNumberRows();

	// Render background
	GLint estimate_height;
	GLint estimate_width;

	estimate_width = 50;
	for(size_t i = 0; i < columns.size(); ++i)
		estimate_width += (GLint)columns[i].width;

	estimate_height = 3 + (GLint)numrows;
	if (m->path.size() > 1)
		estimate_height += 2;
	estimate_height = 20*estimate_height;

	glDisable(GL_TEXTURE_2D);
	glColor4ub(0,0,0,128);
	glBegin(GL_QUADS);
	glVertex2i(0, g_yres);
	glVertex2i(estimate_width, g_yres);
	glVertex2i(estimate_width, g_yres-estimate_height);
	glVertex2i(0, g_yres-estimate_height);
	glEnd();
	glEnable(GL_TEXTURE_2D);

	// Print table and column titles
	glPushMatrix();
	glTranslatef(2.0f, g_yres - 20.0f, 0.0f );
	glScalef(1.0f, -1.0f, 1.0f);
	glColor3ub(255, 255, 255);

	glPushMatrix();
	glwprintf(L"%hs", table->GetTitle().c_str());
	glPopMatrix();
	glTranslatef( 20.0f, 20.0f, 0.0f );

	glPushMatrix();
	for(size_t col = 0; col < columns.size(); ++col)
	{
		glPushMatrix();
		glwprintf(L"%hs", columns[col].title.c_str());
		glPopMatrix();
		glTranslatef(columns[col].width, 0, 0);
	}
	glPopMatrix();
	glTranslatef( 0.0f, 20.0f, 0.0f );

	// Print rows
	int currentExpandId = 1;

	for(size_t row = 0; row < numrows; ++row)
	{
		glPushMatrix();

		if (table->IsHighlightRow(row))
			glColor3ub(255, 128, 128);
		else
			glColor3ub(255, 255, 255);

		if (table->GetChild(row))
		{
			glPushMatrix();
			glTranslatef( -15.0f, 0.0f, 0.0f );
			glwprintf(L"%d", currentExpandId);
			glPopMatrix();
			currentExpandId++;
		}

		for(size_t col = 0; col < columns.size(); ++col)
		{
			glPushMatrix();
			glwprintf(L"%hs", table->GetCellText(row, col).c_str());
			glPopMatrix();
			glTranslatef(columns[col].width, 0, 0);
		}

		glPopMatrix();
		glTranslatef( 0.0f, 20.0f, 0.0f );
	}
	glColor3ub(255, 255, 255);

	if (m->path.size() > 1)
	{
		glTranslatef( 0.0f, 20.0f, 0.0f );
		glPushMatrix();
		glPushMatrix();
		glTranslatef( -15.0f, 0.0f, 0.0f );
		glwprintf( L"0" );
		glPopMatrix();
		glwprintf( L"back to parent" );
		glPopMatrix();
	}

	glPopMatrix();

	glDisable(GL_BLEND);
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

		int k = ev->ev.key.keysym.sym - SDLK_0;
		if (k >= 0 && k <= 9)
		{
			m->NavigateTree(k);
			return IN_HANDLED;
		}
		break;
	}
	case SDL_HOTKEYDOWN:
		if( ev->ev.user.code == HOTKEY_PROFILE_TOGGLE )
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
		else if( ev->ev.user.code == HOTKEY_PROFILE_SAVE )
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
void CProfileViewer::AddRootTable(AbstractProfileTable* table)
{
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

			// Add column headers to 'data'
			for (std::vector<ProfileColumn>::const_iterator col_it = table->GetColumns().begin();
					col_it != table->GetColumns().end(); ++col_it)
				data.push_back(col_it->title);

			// Recursively add all profile data to 'data'
			WriteRows(1, table, data);

			// Calculate the width of each column ( = the maximum width of
			// any value in that column)
			std::vector<size_t> columnWidths;
			size_t cols = table->GetColumns().size();
			for (size_t c = 0; c < cols; ++c)
			{
				size_t max = 0;
				for (size_t i = c; i < data.size(); i += cols)
					max = std::max(max, data[i].length());
				columnWidths.push_back(max);
			}

			// Output data as a formatted table:

			f << "\n\n" << table->GetTitle() << "\n";

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
			for (size_t r = 0; r < table->GetNumberRows(); ++r)
			{
				// Do pretty tree-structure indenting
				CStr indentation = CStr::Repeat("| ", indent-1);
				if (r+1 == table->GetNumberRows())
					indentation += "'-";
				else
					indentation += "|-";

				for (size_t c = 0; c < table->GetColumns().size(); ++c)
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
		fs::wpath path(psLogDir()/L"profile.txt");
		m->outputStream.open(UTF8_from_wstring(path.string()).c_str(), std::ofstream::out | std::ofstream::trunc);

		if (m->outputStream.fail())
		{
			LOG(CLogger::Error, LOG_CATEGORY, L"Failed to open profile log file");
			return;
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
