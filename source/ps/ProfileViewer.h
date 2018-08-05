/* Copyright (C) 2018 Wildfire Games.
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
 * Viewing profiling information (timing and other statistics)
 */

#ifndef INCLUDED_PROFILE_VIEWER
#define INCLUDED_PROFILE_VIEWER

#include "lib/input.h"
#include "ps/CStr.h"
#include "ps/Singleton.h"

class ScriptInterface;

namespace JS
{
	class Value;
}

/**
 * Struct ProfileColumn: Describes one column of an AbstractProfileTable.
 */
struct ProfileColumn
{
	/// Title of the column
	CStr title;

	/// Recommended width of the column, in pixels.
	size_t width;

	ProfileColumn(const CStr& t, size_t w) : title(t), width(w) { }
};


/**
 * Class AbstractProfileTable: Profile table data model.
 *
 * Clients that wish to display debug information in the profile viewer
 * have to implement this class and hook it into CProfileViewer.
 *
 * Note that the profiling system is robust against deletion of
 * object instances in the sense that it will automatically remove
 * an AbstractProfileTable instance from its internal records when
 * you delete it.
 * Conversely, deleting an AbstractProfileTable instance is the responsibility
 * of its creator.
 */
class AbstractProfileTable
{
public:
	virtual ~AbstractProfileTable();

	/**
	 * GetName: Short descriptive name of this table (should be static).
	 *
	 * @return Descriptive name of this table.
	 */
	virtual CStr GetName() = 0;

	/**
	 * GetTitle: Longer, explanatory text (can be dynamic).
	 *
	 * @return Title for the table.
	 */
	virtual CStr GetTitle() = 0;


	/**
	 * GetNumberRows
	 *
	 * @return Number of rows in this table.
	 */
	virtual size_t GetNumberRows() = 0;

	/**
	 * GetColumnDescriptions
	 *
	 * @return A vector describing all columns of the table.
	 */
	virtual const std::vector<ProfileColumn>& GetColumns() = 0;

	/**
	 * GetCellText
	 *
	 * @param row Row index (the first row has index 0).
	 * @param col Column index (the first column has index 0).
	 *
	 * @return Text to be displayed in the given cell.
	 */
	virtual CStr GetCellText(size_t row, size_t col) = 0;

	/**
	 * GetChild: Return a row's child table if the child is expandable.
	 *
	 * @param row Row index (the first row has index 0).
	 *
	 * @return Pointer to the child table if the given row has one.
	 * Otherwise, return 0.
	 */
	virtual AbstractProfileTable* GetChild(size_t row) = 0;

	/**
	 * IsHighlightRow
	 *
	 * @param row Row index (the first row has index 0).
	 *
	 * @return true if the row should be highlighted in a special color.
	 */
	virtual bool IsHighlightRow(size_t row) { UNUSED2(row); return false; }
};


struct CProfileViewerInternals;

/**
 * Class CProfileViewer: Manage and display profiling tables.
 */
class CProfileViewer : public Singleton<CProfileViewer>
{
	friend class AbstractProfileTable;

public:
	CProfileViewer();
	~CProfileViewer();

	/**
	 * RenderProfile: Render the profile display using OpenGL if the user
	 * has enabled it.
	 */
	void RenderProfile();

	/**
	 * Input: Filter and handle any input events that the profile display
	 * is interested in.
	 *
	 * In particular, this function handles enable/disable of the profile
	 * display as well as navigating the information tree.
	 *
	 * @param ev The incoming event.
	 *
	 * @return IN_PASS or IN_HANDLED depending on whether the event relates
	 * to the profiling display.
	 */
	InReaction Input(const SDL_Event_* ev);

	/**
	 * AddRootTable: Add a new profile table as a root table (i.e. the
	 * tables that you cycle through via the profile hotkey).
	 *
	 * @note Tables added via this function are automatically removed from
	 * the list of root tables when they are deleted.
	 *
	 * @param table This table is added as a root table.
	 * @param front If true then the table will be the new first in the list,
	 * else it will be the last.
	 */
	void AddRootTable(AbstractProfileTable* table, bool front = false);

	/**
	 * InputThunk: Delegate to the singleton's Input() member function
	 * if the singleton has been initialized.
	 *
	 * This allows our input handler to be installed via in_add_handler
	 * like a normal, global function input handler.
	 */
	static InReaction InputThunk(const SDL_Event_* ev);

	/**
	 * SaveToFile: Save the current profiler data (for all profile tables)
	 * to a file in the 'logs' directory.
	 */
	void SaveToFile();

	/**
	 * ShowTable: Set the named profile table to be the displayed one. If it
	 * is not found, no profile is displayed.
	 *
	 * @param table The table name (matching AbstractProfileTable::GetName),
	 * or the empty string to display no table.
	 */
	void ShowTable(const CStr& table);

private:
	CProfileViewerInternals* m;
};

#define g_ProfileViewer CProfileViewer::GetSingleton()

#endif
