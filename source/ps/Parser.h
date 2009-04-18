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
Customizeable Text Parser

--Overview-- 

CParserValue		Data! (an int, real, string etc), basically an argument
CParserTaskType		Syntax description for a line (ex. "variable=value")
CParserLine			Parse _one_ line
CParser				Include all syntax (CParserTaskTypes)

The whole CParser* class group is used to read in config files and
give instruction on how that should be made. The CParserTaskType
declares what in a line is arguments, of course different CParserTaskTypes
will exist, and it's up to the system to figure out which one acquired.


--More Info--

	http://forums.wildfiregames.com/0ad/index.php?showtopic=134

*/

#ifndef INCLUDED_PARSER
#define INCLUDED_PARSER

#include "Pyrogenesis.h"

#if MSC_VERSION
#pragma warning(disable:4786)
#endif

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include <vector>
#include <string>
#include <map>
#include <deque>
#include <cmath>

#include "CStr.h"

//-------------------------------------------------
// Types
//-------------------------------------------------

enum _ParserValueType
{
	typeIdent,
	typeValue,
	typeRest,
	typeAddArg,
	typeNull
};

//-------------------------------------------------
// Declarations
//-------------------------------------------------

class CParserValue;
class CParserTaskType;
class CParserLine;
class CParser;


// CParserValue
// ---------------------------------------------------------------------
// A parser value represents an argument
// color=r, g, b
// r, g and b will be CParserValues, or the arguments as they are called.
// This class can store only a string, but can try parsing it to different
// types
class CParserValue
{
public:
	CParserValue();
	~CParserValue();

	// return is error status
	bool GetString(std::string &ret);
	bool GetString( CStr& ret );
	bool GetBool(bool &ret);
	bool GetChar(char &ret);	// As number! otherwise use GetString make sure size=1
	bool GetShort(short &ret);
	bool GetInt(int &ret);
	bool GetLong(long &ret);
	bool GetUnsignedShort(unsigned short &ret);
	bool GetUnsignedInt(unsigned int &ret);
	bool GetUnsignedLong(unsigned long &ret);
	bool GetFloat(float &ret);
	bool GetDouble(double &ret);

	// Memory regardless if it's an int, real, string or whatever
	std::string	m_String;
};

// CParserTaskTypeNode
// ---------------------------------------------------------------------| Class
// A task type is basically a tree, this is because dynamic arguments
//  requires alternative routes, so basically it's a binary tree with an
//  obligatory next node (if it's not the end of the tree) and an alternative
//  dynamic arguments node
//
// If we are at the beginning of this string, this will be the layout of the node
// "<$value_>:_"
//
// m_Element	":"
// m_AltNode	=> "$value"
// m_NextNode	=> "_"
//
class CParserTaskTypeNode
{
public:
	CParserTaskTypeNode();
	~CParserTaskTypeNode();

	// Free node pointers that are below this
	void DeleteChildren();

	// Either the node is a letter or a type, if m_Letter is '\0'
	//  then check m_Type what it is
	char							m_Letter;
	_ParserValueType				m_Type;
	std::string						m_String;	// Used for diverse storage
												//  mainly for the typeAddArg
	
	// Parent node
	CParserTaskTypeNode				*m_ParentNode;

	// Next node 
	CParserTaskTypeNode				*m_NextNode;

	// true means AltNode can be looped <...>
	// false means AltNode is just an optional part [...]
	bool							m_AltNodeRepeatable;

	// Whenever a dynamic argument is used, its first node is stored in this
	//  as an alternative node. The parser first checks if there is an
	//  alternative route, and if it applies to the next node. If not, proceed
	//  as usual with m_String and the next node
	CParserTaskTypeNode				*m_AltNode;

	// There are different kinds of alternative routes
	//int								m_AltRouteType;
};

// CParserTaskType
// ---------------------------------------------------------------------| Class
// A task type is basically different kinds of lines for the parser
// variable=value is one type...
class CParserTaskType
{
public:
	CParserTaskType();
	~CParserTaskType();

	// Delete the whole tree
	void DeleteTree();

	CParserTaskTypeNode				*m_BaseNode;

	// Something to identify it with
	std::string						m_Name;
};

// CParserLine
// ---------------------------------------------------------------------| Class
// Representing one line, i.e. one task, in a config file
class CParserLine
{
public:
	CParserLine();
	~CParserLine();

	std::deque<CParserValue>			m_Arguments;
	bool							m_ParseOK;		// same as ParseString will return
	std::string						m_TaskTypeName;	// Name of the task type found

protected:
	bool ClearArguments();

public:
	// Interface
	bool ParseString(const CParser& parser, const std::string &line);

	// Methods for getting arguments
	//  it returns success
	bool GetArgString			(size_t arg, std::string &ret);
	bool GetArgBool				(size_t arg, bool &ret);
	bool GetArgChar				(size_t arg, char &ret);
	bool GetArgShort			(size_t arg, short &ret);
	bool GetArgInt				(size_t arg, int &ret);
	bool GetArgLong				(size_t arg, long &ret);
	bool GetArgUnsignedShort	(size_t arg, unsigned short &ret);
	bool GetArgUnsignedInt		(size_t arg, unsigned int &ret);
	bool GetArgUnsignedLong		(size_t arg, unsigned long &ret);
	bool GetArgFloat			(size_t arg, float &ret);
	bool GetArgDouble			(size_t arg, double &ret);

	// Get Argument count
	size_t GetArgCount() const { return m_Arguments.size(); }
};

// CParser
// ---------------------------------------------------------------------| Class
// Includes parsing instruction, i.e. task-types
class CParser
{
public:
	CParser();
	~CParser();

	std::vector<CParserTaskType>	m_TaskTypes;

	// Interface
	bool InputTaskType(const std::string& strName, const std::string& strSyntax);
};



// CParserCache
// ---------------------------------------------------------------------| Class
// Provides access to CParser objects, caching them to avoid
// reconstructing the object every time a string needs to be parsed.
class CParserCache
{
public:
	// Returns a simple parser based on a single nameless task-type
	static CParser& Get(const char* str);

private:
	// Self-destructing std::map
	template <typename T, typename P> class SDMap : public std::map<T,P>
	{
	public:
		typedef typename std::map<T,P>::iterator iterator;
		~SDMap()
		{
			for (iterator it = this->begin(); it != this->end(); ++it) delete it->second;
		}
	};

	typedef SDMap<std::string, CParser*> CacheType;

	static CacheType m_Cached;
};

#endif
