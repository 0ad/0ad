/**
 * =========================================================================
 * File        : ProfileViewer.cpp
 * Project     : Pyrogenesis
 * Description : Implementation of profile display (containing only display
 *             : routines, the data model(s) are implemented elsewhere).
 *
 * @author Mark Thompson <mark@wildfiregames.com>
 * @author Nicolai Haehnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#include "precompiled.h"
#include "ProfileViewer.h"
#include "Profile.h"
#include "Renderer.h"
#include "lib/res/graphics/unifont.h"
#include "Hotkey.h"


extern int g_xres, g_yres;

struct CProfileViewerInternals
{
	/// Whether the profiling display is currently visible
	bool profileVisible;

	/// List of root tables
	std::vector<AbstractProfileTable*> rootTables;
	
	/// Path from a root table (path[0]) to the currently visible table (path[size-1])
	std::vector<AbstractProfileTable*> path;
	
	/// Helper functions
	void TableIsDeleted(AbstractProfileTable* table);
	void NavigateTree(int id);
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
	for(int idx = rootTables.size()-1; idx >= 0; --idx)
	{
		if (rootTables[idx] == table)
			rootTables.erase(rootTables.begin() + idx);
	}
	
	for(int idx = 0; (uint)idx < path.size(); ++idx)
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
		int numrows = table->GetNumberRows();
		
		for(int row = 0; row < numrows; ++row)
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

	AbstractProfileTable* table = m->path[m->path.size() - 1];
	const std::vector<ProfileColumn>& columns = table->GetColumns();
	uint numrows = table->GetNumberRows();
	
	// Render background
	int estimate_height;
	int estimate_width;
	
	estimate_width = 50;
	for(uint i = 0; i < columns.size(); ++i)
		estimate_width += columns[i].width;
	
	estimate_height = 3 + numrows;
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
	glwprintf(L"%s", table->GetTitle().c_str());
	glPopMatrix();
	glTranslatef( 20.0f, 20.0f, 0.0f );
	
	glPushMatrix();
	for(uint col = 0; col < columns.size(); ++col)
	{
		glPushMatrix();
		glwprintf(L"%s", columns[col].title.c_str());
		glPopMatrix();
		glTranslatef(columns[col].width, 0, 0);
	}
	glPopMatrix();
	glTranslatef( 0.0f, 20.0f, 0.0f );

	// Print rows
	int currentExpandId = 1;
	
	for(uint row = 0; row < numrows; ++row)
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

		for(uint col = 0; col < columns.size(); ++col)
		{
			glPushMatrix();
			glwprintf(L"%s", table->GetCellText(row, col).c_str());
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
}


// Handle input
InReaction CProfileViewer::Input(const SDL_Event* ev)
{
	switch(ev->type)
	{
	case SDL_KEYDOWN:
	{
		if (!m->profileVisible)
			break;
			
		int k = ev->key.keysym.sym - SDLK_0;
		if (k >= 0 && k <= 9)
		{
			m->NavigateTree(k);
			return IN_HANDLED;
		}
		break;
	}
	case SDL_HOTKEYDOWN:
		if( ev->user.code == HOTKEY_PROFILE_TOGGLE )
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
				uint i;
				
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
		break;
	}
	return( IN_PASS );
}

InReaction CProfileViewer::InputThunk(const SDL_Event* ev)
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

