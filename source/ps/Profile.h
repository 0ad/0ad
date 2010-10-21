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
 * GPG3-style hierarchical profiler
 */

#ifndef INCLUDED_PROFILE
#define INCLUDED_PROFILE

#include <vector>
#include <map>
#include "Singleton.h"

#define PROFILE_AMORTIZE
#define PROFILE_AMORTIZE_FRAMES 50

class CProfileManager;
class CProfileNodeTable;

class CStr8;
class CStrW;

class CProfileNode
{
	friend class CProfileManager;
	friend class CProfileNodeTable;
	
	const char* name;

	int calls_total;
	int calls_frame_current;
#ifdef PROFILE_AMORTIZE
	int calls_frame_buffer[PROFILE_AMORTIZE_FRAMES];
	int* calls_frame_last;
	float calls_frame_amortized;
#else
	int calls_frame_last;
#endif

	double time_total;
	double time_frame_current;
#ifdef PROFILE_AMORTIZE
	double time_frame_buffer[PROFILE_AMORTIZE_FRAMES];
	double* time_frame_last;
	double time_frame_amortized;
#else
	double time_frame_last;
#endif

	long mallocs_total;
	long mallocs_frame_current;
	long mallocs_frame_last;

	double start;
	long start_mallocs;
	int recursion;

	CProfileNode* parent;
	std::vector<CProfileNode*> children;
	std::vector<CProfileNode*> script_children;
	CProfileNodeTable* display_table;
	
public:
	typedef std::vector<CProfileNode*>::iterator profile_iterator;
	typedef std::vector<CProfileNode*>::const_iterator const_profile_iterator;

	CProfileNode( const char* name, CProfileNode* parent );
	~CProfileNode();

	const char* GetName() const { return( name ); }
	int GetCalls() const { return( calls_total ); }
	double GetTime() const { return( time_total ); }

#ifdef PROFILE_AMORTIZE
	float GetFrameCalls() const { return( calls_frame_amortized / PROFILE_AMORTIZE_FRAMES ); }
	double GetFrameTime() const { return( time_frame_amortized / PROFILE_AMORTIZE_FRAMES ); }
#else
	int GetFrameCalls() const { return( calls_frame_last ); }
	double GetFrameTime() const { return( time_frame_last ); }
#endif
	long GetFrameMallocs() const { return( mallocs_frame_last ); }

	const CProfileNode* GetChild( const char* name ) const;
	const CProfileNode* GetScriptChild( const char* name ) const;
	const std::vector<CProfileNode*>* GetChildren() const { return( &children ); }
	const std::vector<CProfileNode*>* GetScriptChildren() const { return( &script_children ); }

	bool CanExpand();

	CProfileNode* GetChild( const char* name );
	CProfileNode* GetScriptChild( const char* name );
	CProfileNode* GetParent() const { return( parent ); }

	// Resets timing information for this node and all its children
	void Reset();
	// Resets frame timings for this node and all its children
	void Frame();
	// Enters the node
	void Call();
	// Leaves the node. Returns true if the node has actually been left
	bool Return();
};

class CProfileManager : public Singleton<CProfileManager>
{
	CProfileNode* root;
	CProfileNode* current;
	double start;
	double frame_start;
	long start_mallocs;
	long frame_start_mallocs;
	std::map<CStr8, const char*> m_internedStrings;

public:
	CProfileManager();
	~CProfileManager();

	// Begins timing for a named subsection
	void Start( const char* name );
	void StartScript( const char* name );

	// Ends timing for the current subsection
	void Stop();

	// Resets all timing information
	void Reset();
	// Resets frame timing information
	void Frame();
	// Resets absolutely everything
	void StructuralReset();

	const char* InternString( const CStr8& intern );

	inline const CProfileNode* GetCurrent() { return( current ); }
	inline const CProfileNode* GetRoot() { return( root ); }
};

#define g_Profiler CProfileManager::GetSingleton()

class CProfileSample
{
public:
	CProfileSample( const char* name )
	{
		if (CProfileManager::IsInitialised())
			g_Profiler.Start( name );
	}
	~CProfileSample()
	{
		if (CProfileManager::IsInitialised())
			g_Profiler.Stop();
	}
};

class CProfileSampleScript
{
public:
	CProfileSampleScript( const char* name )
	{
		if (CProfileManager::IsInitialised())
			g_Profiler.StartScript( name );
	}
	~CProfileSampleScript()
	{
		if (CProfileManager::IsInitialised())
			g_Profiler.Stop();
	}
};

// Put a PROFILE( xyz ) block at the start of all code to be profiled.
// Profile blocks last until the end of the containing scope.
#define PROFILE( name ) CProfileSample __profile( name )
// Cheat a bit to make things slightly easier on the user
#define PROFILE_START( name ) { CProfileSample __profile( name )
#define PROFILE_END( name ) }

#endif // INCLUDED_PROFILE
