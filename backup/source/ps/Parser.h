/*
Customizeable Text Parser
by Gee
Gee@pyro.nu

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

#ifndef __PARSER_H
#define __PARSER_H

#include "Prometheus.h"

#pragma warning(disable:4786)

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include <vector>
#include <string>
#include <map>
#include <deque>
#include <cmath>

//-------------------------------------------------
// Types
//-------------------------------------------------

enum _ParserValueType
{
	typeIdent,
	typeValue,
	typeRest,
	typeAddArg
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
	_bool GetString(std::string &ret);
	_bool GetBool(_bool &ret);
	_bool GetChar(_char &ret);	// As number! otherwise use GetString make sure size=1
	_bool GetShort(_short &ret);
	_bool GetInt(_int &ret);
	_bool GetLong(_long &ret);
	_bool GetUnsignedShort(_ushort &ret);
	_bool GetUnsignedInt(_uint &ret);
	_bool GetUnsignedLong(_ulong &ret);
	_bool GetFloat(float &ret);
	_bool GetDouble(double &ret);

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

	// Either the node is a letter or a type, if m_Leter is '\0'
	//  then check m_Type what it is
	_char							m_Letter;
	_ParserValueType				m_Type;
	std::string						m_String;	// Used for diverse storage
												//  mainly for the typeAddArg
	
	// Parent node
	CParserTaskTypeNode				*m_ParentNode;

	// Next node 
	CParserTaskTypeNode				*m_NextNode;

	// true means AltNode can be looped <...>
	// false means AltNode is just an optional part [...]
	_bool							m_AltNodeRepeatable;

	// Whenever a dynamic argument is used, it's first node is stored in this
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
	_bool							m_ParseOK;		// same as ParseString will return
	std::string						m_TaskTypeName;	// Name of the task type found

protected:
	_bool ClearArguments();

public:
	// Interface
	_bool ParseString(const CParser& parser, std::string line);

	// Methods for getting arguments
	//  it returns success
	_bool GetArgString			(const _int& arg, std::string &ret);
	_bool GetArgBool				(const _int& arg, _bool &ret);
	_bool GetArgChar				(const _int& arg, _char &ret);
	_bool GetArgShort			(const _int& arg, _short &ret);
	_bool GetArgInt				(const _int& arg, _int &ret);
	_bool GetArgLong				(const _int& arg, _long &ret);
	_bool GetArgUnsignedShort	(const _int& arg, _ushort &ret);
	_bool GetArgUnsignedInt		(const _int& arg, _uint &ret);
	_bool GetArgUnsignedLong		(const _int& arg, _ulong &ret);
	_bool GetArgFloat			(const _int& arg, float &ret);
	_bool GetArgDouble			(const _int& arg, double &ret);

	// Get Argument count
	size_t GetArgCount() const { return m_Arguments.size(); }
};

// CParser
// ---------------------------------------------------------------------| Class
// Includes parsing instruction, i.e. task-types
class CParser
{
public:
	CParser::CParser();
	CParser::~CParser();

	std::vector<CParserTaskType>	m_TaskTypes;

	// Interface
	_bool InputTaskType(const std::string& strName, const std::string& strSyntax);
};


#endif
