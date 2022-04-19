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

/*
 * GPG3-style hierarchical profiler
 */

#include "precompiled.h"

#include "Profile.h"
#include "ProfileViewer.h"
#include "ThreadUtil.h"

#include "lib/timer.h"

#include <numeric>

///////////////////////////////////////////////////////////////////////////////////////////////
// CProfileNodeTable



/**
 * Class CProfileNodeTable: Implement ProfileViewer's AbstractProfileTable
 * interface in order to display profiling data in-game.
 */
class CProfileNodeTable : public AbstractProfileTable
{
public:
	CProfileNodeTable(CProfileNode* n);
	virtual ~CProfileNodeTable();

	// Implementation of AbstractProfileTable interface
	virtual CStr GetName();
	virtual CStr GetTitle();
	virtual size_t GetNumberRows();
	virtual const std::vector<ProfileColumn>& GetColumns();

	virtual CStr GetCellText(size_t row, size_t col);
	virtual AbstractProfileTable* GetChild(size_t row);
	virtual bool IsHighlightRow(size_t row);

private:
	/**
	 * struct ColumnDescription: The only purpose of this helper structure
	 * is to provide the global constructor that sets up the column
	 * description.
	 */
	struct ColumnDescription
	{
		std::vector<ProfileColumn> columns;

		ColumnDescription()
		{
			columns.push_back(ProfileColumn("Name", 230));
			columns.push_back(ProfileColumn("calls/frame", 80));
			columns.push_back(ProfileColumn("msec/frame", 80));
			columns.push_back(ProfileColumn("calls/turn", 80));
			columns.push_back(ProfileColumn("msec/turn", 80));
		}
	};

	/// The node represented by this table
	CProfileNode* node;

	/// Columns description (shared by all instances)
	static ColumnDescription columnDescription;
};

CProfileNodeTable::ColumnDescription CProfileNodeTable::columnDescription;


// Constructor/Destructor
CProfileNodeTable::CProfileNodeTable(CProfileNode* n)
{
	node = n;
}

CProfileNodeTable::~CProfileNodeTable()
{
}

// Short name (= name of profile node)
CStr CProfileNodeTable::GetName()
{
	return node->GetName();
}

// Title (= explanatory text plus time totals)
CStr CProfileNodeTable::GetTitle()
{
	char buf[512];
	sprintf_s(buf, ARRAY_SIZE(buf), "Profiling Information for: %s (Time in node: %.3f msec/frame)", node->GetName(), node->GetFrameTime() * 1000.0f );
	return buf;
}

// Total number of children
size_t CProfileNodeTable::GetNumberRows()
{
	return node->GetChildren()->size() + node->GetScriptChildren()->size() + 1;
}

// Column description
const std::vector<ProfileColumn>& CProfileNodeTable::GetColumns()
{
	return columnDescription.columns;
}

// Retrieve cell text
CStr CProfileNodeTable::GetCellText(size_t row, size_t col)
{
	CProfileNode* child;
	size_t nrchildren = node->GetChildren()->size();
	size_t nrscriptchildren = node->GetScriptChildren()->size();
	char buf[256] = "?";

	if (row < nrchildren)
		child = (*node->GetChildren())[row];
	else if (row < nrchildren + nrscriptchildren)
		child = (*node->GetScriptChildren())[row - nrchildren];
	else if (row > nrchildren + nrscriptchildren)
		return "!bad row!";
	else
	{
		// "unlogged" row
		if (col == 0)
			return "unlogged";
		else if (col == 1)
			return "";
		else if (col == 4)
			return "";

		double unlogged_time_frame = node->GetFrameTime();
		double unlogged_time_turn = node->GetTurnTime();
		CProfileNode::const_profile_iterator it;

		for (it = node->GetChildren()->begin(); it != node->GetChildren()->end(); ++it)
		{
			unlogged_time_frame -= (*it)->GetFrameTime();
			unlogged_time_turn -= (*it)->GetTurnTime();
		}
		for (it = node->GetScriptChildren()->begin(); it != node->GetScriptChildren()->end(); ++it)
		{
			unlogged_time_frame -= (*it)->GetFrameTime();
			unlogged_time_turn -= (*it)->GetTurnTime();
		}

		// The root node can't easily count per-turn values (since Turn isn't called until
		// halfway though a frame), so just reset them the zero to prevent weird displays
		if (!node->GetParent())
		{
			unlogged_time_turn = 0.0;
		}

		if (col == 2)
			sprintf_s(buf, ARRAY_SIZE(buf), "%.3f", unlogged_time_frame * 1000.0f);
		else if (col == 4)
			sprintf_s(buf, ARRAY_SIZE(buf), "%.3f", unlogged_time_turn * 1000.f);

		return CStr(buf);
	}

	switch(col)
	{
	default:
	case 0:
		return child->GetName();

	case 1:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.1f", child->GetFrameCalls());
		break;
	case 2:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.3f", child->GetFrameTime() * 1000.0f);
		break;
	case 3:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.1f", child->GetTurnCalls());
		break;
	case 4:
		sprintf_s(buf, ARRAY_SIZE(buf), "%.3f", child->GetTurnTime() * 1000.0f);
		break;
	}
	return CStr(buf);
}

// Return a pointer to the child table if the child node is expandable
AbstractProfileTable* CProfileNodeTable::GetChild(size_t row)
{
	CProfileNode* child;
	size_t nrchildren = node->GetChildren()->size();
	size_t nrscriptchildren = node->GetScriptChildren()->size();

	if (row < nrchildren)
		child = (*node->GetChildren())[row];
	else if (row < nrchildren + nrscriptchildren)
		child = (*node->GetScriptChildren())[row - nrchildren];
	else
		return 0;

	if (child->CanExpand())
		return child->display_table;

	return 0;
}

// Highlight all script nodes
bool CProfileNodeTable::IsHighlightRow(size_t row)
{
	size_t nrchildren = node->GetChildren()->size();
	size_t nrscriptchildren = node->GetScriptChildren()->size();

	return (row >= nrchildren && row < (nrchildren + nrscriptchildren));
}

///////////////////////////////////////////////////////////////////////////////////////////////
// CProfileNode implementation


// Note: As with the GPG profiler, name is assumed to be a pointer to a constant string; only pointer equality is checked.
CProfileNode::CProfileNode( const char* _name, CProfileNode* _parent )
{
	name = _name;
	recursion = 0;

	Reset();

	parent = _parent;

	display_table = new CProfileNodeTable(this);
}

CProfileNode::~CProfileNode()
{
	profile_iterator it;
	for( it = children.begin(); it != children.end(); ++it )
		delete( *it );
	for( it = script_children.begin(); it != script_children.end(); ++it )
		delete( *it );

	delete display_table;
}

template<typename T>
static double average(const T& collection)
{
	if (collection.empty())
		return 0.0;
	return std::accumulate(collection.begin(), collection.end(), 0.0) / collection.size();
}

double CProfileNode::GetFrameCalls() const
{
	return average(calls_per_frame);
}

double CProfileNode::GetFrameTime() const
{
	return average(time_per_frame);
}

double CProfileNode::GetTurnCalls() const
{
	return average(calls_per_turn);
}

double CProfileNode::GetTurnTime() const
{
	return average(time_per_turn);
}

const CProfileNode* CProfileNode::GetChild( const char* childName ) const
{
	const_profile_iterator it;
	for( it = children.begin(); it != children.end(); ++it )
		if( (*it)->name == childName )
			return( *it );

	return( NULL );
}

const CProfileNode* CProfileNode::GetScriptChild( const char* childName ) const
{
	const_profile_iterator it;
	for( it = script_children.begin(); it != script_children.end(); ++it )
		if( (*it)->name == childName )
			return( *it );

	return( NULL );
}

CProfileNode* CProfileNode::GetChild( const char* childName )
{
	profile_iterator it;
	for( it = children.begin(); it != children.end(); ++it )
		if( (*it)->name == childName )
			return( *it );

	CProfileNode* newNode = new CProfileNode( childName, this );
	children.push_back( newNode );
	return( newNode );
}

CProfileNode* CProfileNode::GetScriptChild( const char* childName )
{
	profile_iterator it;
	for( it = script_children.begin(); it != script_children.end(); ++it )
		if( (*it)->name == childName )
			return( *it );

	CProfileNode* newNode = new CProfileNode( childName, this );
	script_children.push_back( newNode );
	return( newNode );
}

bool CProfileNode::CanExpand()
{
	return( !( children.empty() && script_children.empty() ) );
}

void CProfileNode::Reset()
{
	calls_per_frame.clear();
	calls_per_turn.clear();
	calls_frame_current = 0;
	calls_turn_current = 0;

	time_per_frame.clear();
	time_per_turn.clear();
	time_frame_current = 0.0;
	time_turn_current = 0.0;

	profile_iterator it;
	for (it = children.begin(); it != children.end(); ++it)
		(*it)->Reset();
	for (it = script_children.begin(); it != script_children.end(); ++it)
		(*it)->Reset();
}

void CProfileNode::Frame()
{
	calls_per_frame.push_back(calls_frame_current);
	time_per_frame.push_back(time_frame_current);

	calls_frame_current = 0;
	time_frame_current = 0.0;

	profile_iterator it;
	for (it = children.begin(); it != children.end(); ++it)
		(*it)->Frame();
	for (it = script_children.begin(); it != script_children.end(); ++it)
		(*it)->Frame();
}

void CProfileNode::Turn()
{
	calls_per_turn.push_back(calls_turn_current);
	time_per_turn.push_back(time_turn_current);

	calls_turn_current = 0;
	time_turn_current = 0.0;

	profile_iterator it;
	for (it = children.begin(); it != children.end(); ++it)
		(*it)->Turn();
	for (it = script_children.begin(); it != script_children.end(); ++it)
		(*it)->Turn();
}

void CProfileNode::Call()
{
	calls_frame_current++;
	calls_turn_current++;
	if (recursion++ == 0)
	{
		start = timer_Time();
	}
}

bool CProfileNode::Return()
{
	if (--recursion != 0)
		return false;

	double now = timer_Time();
	time_frame_current += (now - start);
	time_turn_current += (now - start);
	return true;
}

CProfileManager::CProfileManager() :
	root(NULL), current(NULL), needs_structural_reset(false)
{
	PerformStructuralReset();
}

CProfileManager::~CProfileManager()
{
	delete root;
}

void CProfileManager::Start( const char* name )
{
	if( name != current->GetName() )
		current = current->GetChild( name );
	current->Call();
}

void CProfileManager::StartScript( const char* name )
{
	if( name != current->GetName() )
		current = current->GetScriptChild( name );
	current->Call();
}

void CProfileManager::Stop()
{
	if (current->Return())
		current = current->GetParent();
}

void CProfileManager::Reset()
{
	root->Reset();
}

void CProfileManager::Frame()
{
	root->time_frame_current += (timer_Time() - root->start);

	root->Frame();

	if (needs_structural_reset)
	{
		PerformStructuralReset();
		needs_structural_reset = false;
	}

	root->start = timer_Time();
}

void CProfileManager::Turn()
{
	root->Turn();
}

void CProfileManager::StructuralReset()
{
	// We can't immediately perform the reset, because we're probably already
	// nested inside the profile tree and it will get very confused if we delete
	// the tree when we're not currently at the root.
	// So just set a flag to perform the reset at the end of the frame.

	needs_structural_reset = true;
}

void CProfileManager::PerformStructuralReset()
{
	delete root;
	root = new CProfileNode("root", NULL);
	root->Call();
	current = root;
	g_ProfileViewer.AddRootTable(root->display_table, true);
}

CProfileSample::CProfileSample(const char* name)
{
	if (CProfileManager::IsInitialised())
	{
		// The profiler is only safe to use on the main thread
		if(Threading::IsMainThread())
			g_Profiler.Start(name);
	}
}

CProfileSample::~CProfileSample()
{
	if (CProfileManager::IsInitialised())
		if(Threading::IsMainThread())
			g_Profiler.Stop();
}
