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

#include "precompiled.h"

#include "Parser.h"

#if MSC_VERSION
#pragma warning(disable:4786)
#endif


//-------------------------------------------------
// Macros
//-------------------------------------------------

#define REGULAR_MAX_LENGTH			10
#define START_DYNAMIC				'<'
#define END_DYNAMIC					'>'
#define START_OPTIONAL				'['
#define END_OPTIONAL				']'
#define REGULAR_EXPRESSION			'$'

// use GetDouble and type-cast it to <<type>>
#define FUNC_IMPL_CAST_GETDOUBLE(func_name,type)		\
bool CParserValue::func_name(type &ret)					\
{														\
	double d;											\
	if (GetDouble(d))									\
		return ret = (type)d, true;						\
	else												\
		return false;									\
}

// Function-implementation creator for GetArg%type% that will call
//  Get%type% from the CParserValue
// func_name must belong to CParserFile
#define FUNC_IMPL_GETARG(func_name, get_name, type)		\
bool CParserLine::func_name(size_t arg, type &ret)		\
{														\
	if (GetArgCount() <= arg)							\
		return false;									\
	return m_Arguments[arg].get_name(ret);				\
}

//-------------------------------------------------
// Function definitions
//-------------------------------------------------

static bool _IsStrictNameChar(const char& c);
static bool _IsValueChar(const char& c);


// Functions used for checking a character if it belongs to a value
//  or not

// Checks ident
static bool _IsStrictNameChar(const char& c)
{
	return ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9'));
}

// Checks value
static bool _IsValueChar(const char& c)
{
	return ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			c=='.' || c=='_');
}

// CParserValue
// ---------------------------------------------------------------------| Class

CParserValue::CParserValue()
{
}

CParserValue::~CParserValue()
{
}

// Parse the std::string in Value to different types

// bool
bool CParserValue::GetBool(bool &ret)
{
	// TODO Raj Add or remove some? I can make it all lowercase
	//  first so True and TRUE also works, or you could just
	//  add them too

	// true
	if (m_String == "true" ||
		m_String == "on" ||
		m_String == "1" ||
		m_String == "yes")
	{
		ret = true;
		return true;
	}
	else
	// false
	if (m_String == "false" ||
		m_String == "off" ||
		m_String == "0" ||
		m_String == "no")
	{
		ret = false;
		return true;
	}

	// point only erroneous runs reach
	return false;
}

// double
bool CParserValue::GetDouble(double &ret)
{
	// locals
	double			TempRet = 0.0;
	size_t			Size = m_String.size();
	size_t			i;
	bool			AtLeastOne = false;		// Checked if at least one of the loops
											//  run, otherwise "." would parse OK
	size_t			DecimalPos;
	bool			Negative = false;		// "-" is found

	// Check if '-' is found
	if (m_String[0]=='-')
	{
		Negative = true;
	}
	
	// find decimal position
	DecimalPos = m_String.find(".");
	if (DecimalPos == std::string::npos)	
		DecimalPos = Size;

	// Iterate left of the decimal sign
	//
	for (i=(Negative?1:0); i < DecimalPos; ++i)
	{
		// Set AtLeastOne to true
		AtLeastOne = true;

		// Check if a digit is found
		if (m_String[i] >= '0' && m_String[i] <= '9')
		{
			double exp = (DecimalPos-i-1);	// disambiguate pow() argument type
			TempRet += (m_String[i]-'0')*pow(10.0, exp);
		}
		else
		{
			// parse error!
			return false;
		}
	}

	// Iterate right of the decimal sign
	//
	for (i=DecimalPos+1; i < Size; ++i)
	{
		// Set AtLeastOne to true
		AtLeastOne = true;

		// Check if a digit is found
		if (m_String[i] >= '0' && m_String[i] <= '9')
		{
			double exp = (int)(DecimalPos-i);	// disambiguate pow() argument type
			TempRet += (m_String[i]-'0')*pow(10.0,exp);
		}
		// It will accept and ending f, like 1.0f
		else if (!(i==Size-1 && m_String[i] == 'f'))
		{
			// parse error!
			return false;
		}
	}

	if (!AtLeastOne)return false;

	// Set the reference to the temp value and return success
	ret = (Negative?-TempRet:TempRet);
	return true;
}

// std::string - only return m_String, can't fail
bool CParserValue::GetString(std::string &ret)
{
	ret = m_String;
	return true;
}

bool CParserValue::GetString( CStr& ret )
{
	ret = m_String;
	return true; 
}

// These macros include the IMPLEMENTATION of the 
// the function in the macro argument for CParserValue
// They use GetDouble, and then type-cast it
FUNC_IMPL_CAST_GETDOUBLE(GetFloat,			float)
FUNC_IMPL_CAST_GETDOUBLE(GetChar,			char)
FUNC_IMPL_CAST_GETDOUBLE(GetShort,			short)
FUNC_IMPL_CAST_GETDOUBLE(GetInt,			int)
FUNC_IMPL_CAST_GETDOUBLE(GetLong,			long)
FUNC_IMPL_CAST_GETDOUBLE(GetUnsignedShort,	unsigned short)
FUNC_IMPL_CAST_GETDOUBLE(GetUnsignedInt,	unsigned int)
FUNC_IMPL_CAST_GETDOUBLE(GetUnsignedLong,	unsigned long)

// CParserTaskTypeNode
// ---------------------------------------------------------------------| Class

CParserTaskTypeNode::CParserTaskTypeNode() : m_ParentNode(NULL), m_NextNode(NULL), m_AltNode(NULL)
{
}

CParserTaskTypeNode::~CParserTaskTypeNode()
{
}

// Delete all children
void CParserTaskTypeNode::DeleteChildren()
{
	// Delete nodes if applicable
	if (m_NextNode)
	{
		m_NextNode->DeleteChildren();
		delete m_NextNode;
		m_NextNode = NULL;
	}

	if (m_AltNode)
	{
		m_AltNode->DeleteChildren();
		delete m_AltNode;
		m_AltNode = NULL;
	}
}

// CParserTaskType
// ---------------------------------------------------------------------| Class

CParserTaskType::CParserTaskType() : m_BaseNode(NULL)
{
}

CParserTaskType::~CParserTaskType()
{
}

// Delete m_BaseNode and all of its children
void CParserTaskType::DeleteTree()
{
	if (m_BaseNode)
	{
		m_BaseNode->DeleteChildren();
		delete m_BaseNode;
		m_BaseNode = NULL;
	}
}

// CParserLine
// ---------------------------------------------------------------------| Class

CParserLine::CParserLine()
{
}

CParserLine::~CParserLine()
{
	
	ClearArguments();
}

// Clear arguments (deleting m_Memory
bool CParserLine::ClearArguments()
{
	// Now we can actually clear it
	m_Arguments.clear();
	return true;
}

// Implementation of CParserFile::GetArg*
//  it just checks if argument isn't out of range, and
//  then it uses the the respective function in CParserValue
FUNC_IMPL_GETARG(GetArgString,			GetString,			std::string)
FUNC_IMPL_GETARG(GetArgBool,			GetBool,			bool)
FUNC_IMPL_GETARG(GetArgChar,			GetChar,			char)
FUNC_IMPL_GETARG(GetArgShort,			GetShort,			short)
FUNC_IMPL_GETARG(GetArgInt,				GetInt,				int)
FUNC_IMPL_GETARG(GetArgLong,			GetLong,			long)
FUNC_IMPL_GETARG(GetArgUnsignedShort,	GetUnsignedShort,	unsigned short)
FUNC_IMPL_GETARG(GetArgUnsignedInt,		GetUnsignedInt,		unsigned int)
FUNC_IMPL_GETARG(GetArgUnsignedLong,	GetUnsignedLong,	unsigned long)
FUNC_IMPL_GETARG(GetArgFloat,			GetFloat,			float)
FUNC_IMPL_GETARG(GetArgDouble,			GetDouble,			double)

// ParseString
// ------------------------------------------------------------------| Function
// Parses a line, dividing it into segments according to defined semantics
// each segment is called an argument and represents a value of some kind
// ex:
// variable = 5		=> variable, =, 5
// CallFunc(4,2)	=> CallFunc, 4, 2

// TODO Gee: Make Parser use CStr.
bool CParserLine::ParseString(const CParser& Parser, const std::string &strLine)
{
	// Don't process empty std::string
	if (strLine == std::string())
	{
		m_ParseOK = false;		// Empty lines should never be inputted by CParserFile
		return m_ParseOK;
	}

	// Locals
	bool				Extract=false;
	size_t				ExtractPos=0;
	char				Buffer[256];
	char				Letter[] = {'\0','\0'};		// Letter as std::string
	std::vector<std::string>		Segments;
	std::string				strSub;
	size_t				i;

	// Set result to false, then if a match is found, turn it true
	m_ParseOK = false;

	/* 
	TODO Gee Remove this comment!
	// Remove C++-styled comments!
	// * * * *
	int pos = strLine.find("//");
	if (pos != std::string::npos)
		strLine = strLine.substr(0,pos);
	*/

	// Divide std::string into smaller std::vectors, separators are unusual signs
	// * * * *

	for (i=0; i<strLine.size(); ++i)
	{
		// Check if we're trying to use some kind of type
		if (!Extract)
		{
			// GET NAME, IDENT, FLOAT
			if (_IsValueChar(strLine[i]))
			{
				Extract = true;
				ExtractPos = i;
				memset((void*)Buffer, '\0', sizeof(char)*256);
			}
			else
			// GET STRING BETWEEN QUOTES	
			if (strLine[i] == '\"')
			{
				// Extract a std::string, search for another "
				size_t pos = strLine.find("\"", i+1);

				// If matching can't be found,
				//  the parsing will fail!
				if (pos == std::string::npos)
				{
					// TODO Gee - Output in logfile
					m_ParseOK = false;
					return m_ParseOK;
				}

				// Get substring
				// Add a " to indicate this is a "..." std::string
				//  and can't be used as name
				strSub = "\"" + strLine.substr(i+1, pos-i-1);

				// Input substring!
				Segments.push_back(strSub);

				// Now we can't skip everything that we 
				//  we just read in, update i
				i = pos;
			}
			// GET JUST ONE CHARACTER
			else
			{
				// Input just the one char
				Letter[0] = strLine[i];
				Segments.push_back(Letter);
				continue;
			}
		}
		// Extract whatever
		if (Extract)
		{
			// No type names are longer than 256 characters
			if (i-ExtractPos >= 256)
			{
				Extract=false;
			}
			else
			{
				// Extract std::string after $ !
				// break whenever we reach a sign that's not A-Z a-z
				if (_IsValueChar(strLine[i]))
				{
					Buffer[i-ExtractPos] = strLine[i];
				}
				else
				{
					// Extraction is finished
					Extract=false;

					// strLine[i] is now a non-regular character
					//  we'll jump back one step so that will
					//  be included next loop
					--i;
				}

				// Check if std::string is complete
				if (i == strLine.size()-1)
					Extract=false;
			}

			// If extraction was finished! Input Buffer
			if (Extract == false)
			{
				Segments.push_back( std::string(Buffer) );
			}
		}
	}

	// Try to find an appropriate CParserTaskType in parser
	// * * * *

	// Locals
	size_t Progress;						// progress in Segments index
	size_t Lane=0;							// Have many alternative routes we are in
	bool Match;							// If a task-type match has been found
	// The std::vector of these three represents the different lanes
	//  LastValidProgress[1] takes you back to lane 1 and how
	//  the variables was set at that point
	std::vector<size_t> LastValidProgress;		// When diving into a dynamic argument store store
										//  the last valid so you can go back to it
	std::vector<size_t> LastValidArgCount;		// If an alternative route turns out to fail, we
										//  need to know the amount of arguments on the last
										//  valid position, so we can remove them.
	std::vector<bool> LastValidMatch;		// Match at that point
	bool BlockAltNode = false;			// If this turns true, the alternative route
										//  tested was not a success, and the settings
										//  should be set back in order to test the 
										//  next node instead
	bool LookNoFurther = false;			// If this turns true, it means a definite match has been
										//  found and no further looking is required
	CParserTaskTypeNode *CurNode=NULL;	// Current node on task type
	CParserTaskTypeNode *PrevNode=NULL;	// Last node
	UNUSED2(PrevNode);

	// Iterate all different TaskType, and all TaskTypeElements... 
	//  start from left and go  to the right (prog), comparing 
	//  the similarities. If enough
	//  similarities are found, then we can declare progress as
	//  that type and exit loop
	std::vector<CParserTaskType>::const_iterator cit_tt;
	for (cit_tt = Parser.m_TaskTypes.begin(); 
		 cit_tt != Parser.m_TaskTypes.end(); 
		 ++cit_tt)
	{
		// Reset for this task-type
		Match = true;
		Progress = 0;
		ClearArguments();				// Previous failed can have filled this
		CurNode = cit_tt->m_BaseNode;	// Start at base node
		LookNoFurther = false;
		BlockAltNode = false;

		// This loop will go through the whole tree until
		// it reaches an empty node
		while (!LookNoFurther)
		{
			// Check if node is valid
			//  otherwise try to jump back to parent
			if (CurNode->m_NextNode == NULL &&
				(CurNode->m_AltNode == NULL || BlockAltNode))
			{
				// Jump back to valid
				//CurNode = PrevNode;

				// If the node has no children, it's the last, and we're
				//  on lane 0, i.e. with no 
				if (CurNode->m_NextNode == NULL &&
					(CurNode->m_AltNode == NULL || BlockAltNode) &&
					Lane == 0)
				{
					if (Progress != Segments.size())
						Match = false;

					break;
				}
				else
				{
					CParserTaskTypeNode *OldNode = NULL;

					// Go back to regular route!
					for(;;)
					{
						OldNode = CurNode;
						CurNode = CurNode->m_ParentNode;

						if (CurNode->m_AltNode == OldNode)
						{
							break;
						}
					}

					// If the alternative route isn't repeatable, block alternative route for
					//  next loop cycle
					if (!CurNode->m_AltNodeRepeatable)
						BlockAltNode = true;

					// Decrease lane
					--Lane;
				}
			}

			// Check alternative route
			// * * * *

			// Check if alternative route is present
			//  note, if an alternative node has already failed
			//  we don't want to force usage of the next node
			//  therefore BlockAltNode has to be false
			if (!BlockAltNode)
			{
				if (CurNode->m_AltNode)
				{
					// Alternative route found, we'll test this first!
					CurNode = CurNode->m_AltNode;

					// --- New node is set!

					// Make sure they are large enough
					if (LastValidProgress.size() < Lane+1)
					{
						LastValidProgress.resize(Lane+1);
						LastValidMatch.resize(Lane+1);
						LastValidArgCount.resize(Lane+1);
					}

					// Store last valid progress
					LastValidProgress[Lane] = Progress;
					LastValidMatch[Lane] = Match;
					LastValidArgCount[Lane] = (int)m_Arguments.size();

					++Lane;

					continue;
				}
			}
			else BlockAltNode = false;

			// Now check Regular Next Node
			// * * * *

			if (CurNode->m_NextNode)
			{
				// Important!
				// Change working node to the next node!
				CurNode = CurNode->m_NextNode;

				// --- New node is set!

				// CHECK IF LETTER IS CORRECT
				if (CurNode->m_Letter != '\0')
				{
					// OPTIONALLY SKIP BLANK SPACES
					if (CurNode->m_Letter == '_')
					{
						// Find blank space if any!
						//  and jump to the next non-blankspace
						if (Progress < Segments.size())
						{	
							// Skip blankspaces AND tabs!
							while (Segments[Progress].size()==1 &&
								   (Segments[Progress][0]==' ' || 
									Segments[Progress][0]=='\t'))
							{
								++Progress;

								// Check length
								if (Progress >= Segments.size())
								{
									break;
								}
							}
						}
					}
					else
					// CHECK LETTER IF IT'S CORRECT
					{
						if (Progress < Segments.size())
						{
							// This should be 1-Letter long
							if (Segments[Progress].size() != 1)
								Match = false;

							// Check Letter
							if (CurNode->m_Letter != Segments[Progress][0])
								Match = false;

							// Update progress
							++Progress;
						}
						else Match = false;
					}
				}

				else if (CurNode->m_Type == typeNull)
				{
					// Match without doing anything (leaving Match==true)
				}

				// CHECK NAME
				else
				{
					// Do this first, because we wan't to
					//  avoid the Progress and Segments.size()
					//  check for this
					if (CurNode->m_Type == typeAddArg)
					{
						// Input argument
						CParserValue value;
						value.m_String = CurNode->m_String;
						m_Arguments.push_back(value);
					}
					else
					{
						// Alright! An ident or const has been acquired, if we
						//  can't find any or if the std::string has run out
						//  that invalidates the match

						// String end?
						if (Progress >= Segments.size())
						{
							Match = false;
						}
						else
						{
							// Store argument in CParserValue!
							CParserValue value;
							size_t i;

							switch(CurNode->m_Type)
							{
							case typeIdent:
								// Check if this really is a std::string
								if (!_IsStrictNameChar(Segments[Progress][0]))
								{
									Match = false;
									break;
								}
								
								// Same as at typeValue, but this time
								//  we won't allow strings like "this", just
								//  like this
								if (Segments[Progress][0] == '\"')
									Match = false;
								else
									value.m_String = Segments[Progress];
								
								// Input argument!
								m_Arguments.push_back(value);

								++Progress;
								break;
							case typeValue:
								// Check if this really is a std::string
								if (!_IsValueChar(Segments[Progress][0]) &&
									Segments[Progress][0] != '\"')
								{
									Match = false;
									break;
								}
										
								// Check if initial is -> " <-, because that means it was
								//  stored from a "String like these with quotes"
								//  We don't want to store that prefix
								if (Segments[Progress][0] == '\"')
									value.m_String = Segments[Progress].substr(1, Segments[Progress].size()-1);
								else
									value.m_String = Segments[Progress];
								
								// Input argument!
								m_Arguments.push_back(value);

								++Progress;
								break;
							case typeRest:
								// Extract the whole of the std::string
								
								// Reset, probably is but still
								value.m_String = std::string();

								for (i=Progress; i<Segments.size(); ++i)
								{
									value.m_String += Segments[i];

									// If argument starts with => " <=, add one to the end of it too
									if (Segments[i][0] == '"')
										value.m_String += "\"";
								}

								m_Arguments.push_back(value);

								// Now BREAK EVERYTHING !
								//  We're done, we found our match and let's get out
								LookNoFurther = true;
								//Match = true;
								break;
							default:
								break;
							}
						}
					}
				}
			}

			// Check if match is false! if it is, try returning to last valid state
			if (!Match && Lane > 0)
			{
				// The alternative route failed
				BlockAltNode = true;

				CParserTaskTypeNode *OldNode = NULL;

				// Go back to regular route!
				for(;;)
				{
					OldNode = CurNode;
					CurNode = CurNode->m_ParentNode;

					if (CurNode->m_AltNode == OldNode)
					{
						break;
					}
				}

				// Decrease lane
				--Lane;

				// Restore values as before
				Progress = LastValidProgress[Lane];
				Match = LastValidMatch[Lane];
				m_Arguments.resize(LastValidArgCount[Lane]);
			}
		}

		// Check if it was a match!
		if (Match)
		{
			// Before we celebrate the match, let's check if whole
			//  of Segments has been used, and if so we have to
			//  nullify the match
			//if (Progress == Segments.size())
			{
				// *** REPORT MATCH WAS FOUND ***
				m_TaskTypeName = cit_tt->m_Name;
				m_ParseOK = true;
				break;
			}
		}
	}

	// POST-PROCESSING OF ARGUMENTS!
	
	// if _minus is found as argument, remove it and add "-" to the one after that
	// note, it's easier if std::iterator isn't used here
	
	for (i=1; i<GetArgCount(); ++i)
	{
		if (m_Arguments[i-1].m_String == "_minus")
		{
			// Add "-" to next, and remove "_minus"
			m_Arguments[i].m_String = "-" + m_Arguments[i].m_String;
			m_Arguments.erase(m_Arguments.begin() + (i-1));
		}
	}

	return m_ParseOK;
}

// CParser
// ---------------------------------------------------------------------| Class

// ctor
CParser::CParser()
{
}

// dtor
CParser::~CParser()
{
	// Delete all task type trees
	std::vector<CParserTaskType>::iterator itTT;
	for (itTT = m_TaskTypes.begin();
		 itTT != m_TaskTypes.end();
		 ++itTT)
	{
		itTT->DeleteTree();
	}
}

// InputTaskType
// ------------------------------------------------------------------| Function
// A task-type is a std::string representing the acquired syntax when parsing
//  This function converts that std::string into a binary tree, making it easier
//  and faster to later parse.
bool CParser::InputTaskType(const std::string& strName, const std::string& strSyntax)
{
	// Locals
	CParserTaskType TaskType;	// Object we acquire to create
	char Buffer[REGULAR_MAX_LENGTH];
	size_t ExtractPos = 0;
	bool Extract = false;
	bool Error = false;
	size_t i;
	bool ConstructNew = false;	// If it's the first input, then don't
								//  construct a new node, because we
								//  we already have m_BaseNode

	// Construct base node
	TaskType.m_BaseNode = new CParserTaskTypeNode();

	// Working node
	CParserTaskTypeNode *CurNode = TaskType.m_BaseNode;

	// Loop through the std::string and construct nodes in the binary tree
	//  when applicable
	for (i=0; i<strSyntax.size(); ++i)
	{
		// Extract is a variable that is true when we want to extract
		//  parts that is longer than one character.
		if (!Extract)
		{
			if (strSyntax[i] == REGULAR_EXPRESSION)
			{
				Extract = true;
				ExtractPos = i+1; // Skip $
				memset((void*)Buffer, '\0', sizeof(char)*REGULAR_MAX_LENGTH);

				// We don't want to extract '$' so we'll just continue to next loop run
				continue;
			}
			else
			if (strSyntax[i] == START_DYNAMIC || strSyntax[i] == START_OPTIONAL)
			{

				// Slight hack: because things are stored in a binary tree,
				// it can't handle "<[a][b]>" -- the <...> node has only
				// one slot for an optional [...] node. To avoid this problem,
				// typeNull nodes are used to indicate things that always
				// succeed but can have altnodes attached:
/*
	parent			parent
	  \		===>	  \
	<...>	===>	<...>		<-- CurNode
	/	\			/	\
   /	 \		   /	 \
next	[a]		Null	[a]		<-- added NewNode
				/  \
			  next	[b]
*/
				if (CurNode->m_AltNode)
				{

					// Rearrange the tree, as shown above:

					// Create NewNode
					CParserTaskTypeNode* NewNode = new CParserTaskTypeNode();
					NewNode->m_ParentNode = CurNode;
					NewNode->m_Letter = '\0';
					NewNode->m_Type = typeNull;

					// Copy 'next' into NewNode
					NewNode->m_NextNode = CurNode->m_NextNode;
					// Replace 'next' with NewNode inside CurNode
					CurNode->m_NextNode = NewNode;
		
					// Update CurNode, so the following code inserts into [b]
					CurNode = NewNode;
				}

				// Dive into the alternative node
				debug_assert(! CurNode->m_AltNode);
				CurNode->m_AltNode = new CParserTaskTypeNode();
				CurNode->m_AltNode->m_ParentNode = CurNode;

				// It's repeatable
				CurNode->m_AltNodeRepeatable = bool(strSyntax[i]==START_DYNAMIC);

				// Set to current
				CurNode = CurNode->m_AltNode;

				ConstructNew = false;

				// We're done extracting for now
				continue;
			}
			else
			if (strSyntax[i] == END_DYNAMIC || strSyntax[i] == END_OPTIONAL)
			{
				CParserTaskTypeNode *OldNode = NULL;

				// Jump out of this alternative route
				for(;;)
				{
					OldNode = CurNode;
					CurNode = CurNode->m_ParentNode;

					if (CurNode == NULL)
					{
						// Syntax error
						Error = true;
						break;
					}

					if (CurNode->m_AltNode == OldNode)
					{
						break;
					}
				}
				
				if (Error)break;
			}
			else
			{
				// Check if this is the first input
				// CONSTRUCT A CHILD NODE
				debug_assert(! CurNode->m_NextNode);
				CurNode->m_NextNode = new CParserTaskTypeNode();
				CurNode->m_NextNode->m_ParentNode = CurNode;

				// Jump into !
				CurNode = CurNode->m_NextNode;	

				// Set CurNode
				CurNode->m_Letter = strSyntax[i];		
			}
		}

		// Extact
		if (Extract)
		{
			// No type names are longer than REGULAR_MAX_LENGTH characters
			if (i-ExtractPos >= REGULAR_MAX_LENGTH)
			{
				Extract=false;
			}
			else
			{
				// Extract std::string after $ !
				// break whenever we reach a sign that's not A-Z a-z
				if (_IsStrictNameChar(strSyntax[i]))
				{
					Buffer[i-ExtractPos] = strSyntax[i];
				}
				else
				{
					// Extraction is finished
					Extract=false;

					// strLine[i] is now a non-regular character
					//  we'll jump back one step so that will
					//  be included next loop
					--i;
				}

				// Check if std::string is complete
				if (i == strSyntax.size()-1)
					Extract=false;
			}

			// If extraction was finished! Input Buffer
			if (Extract == false)
			{
				// CONSTRUCT A CHILD NODE
				debug_assert(! CurNode->m_NextNode);
				CurNode->m_NextNode = new CParserTaskTypeNode();
				CurNode->m_NextNode->m_ParentNode = CurNode;

				// Jump into !
				CurNode = CurNode->m_NextNode;					

				CurNode->m_Letter = '\0';

				std::string str = std::string(Buffer);

				// Check value and set up CurNode accordingly
				if (str == "value")		CurNode->m_Type = typeValue;
				else 
				if (str == "ident")		CurNode->m_Type = typeIdent;
				else
				if (str == "rest")		CurNode->m_Type = typeRest;
				else
				if (str == "rbracket")	CurNode->m_Letter = '>';
				else 
				if (str == "lbracket")	CurNode->m_Letter = '<';
				else 
				if (str == "rbrace")	CurNode->m_Letter = ']';
				else 
				if (str == "lbrace")	CurNode->m_Letter = '[';
				else 
				if (str == "dollar")	CurNode->m_Letter = '$';
				else
				if (str == "arg")
				{
					// After $arg, you need a parenthesis, within that parenthesis is a std::string
					//  that will be added as an argument when it's passed through

					CurNode->m_Type = typeAddArg;
					
					// Check length, it has to have place for at least a '(' and ')' after $arg
					if (ExtractPos+4 >= strSyntax.size())
					{
						Error = true;
						break;
					}
					
					// We want to extract what's inside the parenthesis after $arg
					//  if it's not there at all, it's a syntactical error
					if (strSyntax[ExtractPos+3] != '(')
					{
						Error = true;
						break;
					}

					// Now try finding the second ')'
					size_t Pos = strSyntax.find(")", ExtractPos+5);

					// Check if ')' exists at all
					if (Pos == std::string::npos)
					{
						Error = true;
						break;
					}

					// Now extract std::string within ( and )
					CurNode->m_String = strSyntax.substr(ExtractPos+4, Pos-(ExtractPos+4));

					// Now update position
					i = (int)Pos;
				}
				else
				{
					// TODO Gee report in log too
					Error = true;
				}
			}
		}
	}

	// Input TaskType
	if (!Error)
	{
		// Set name and input
		TaskType.m_Name = strName;
		m_TaskTypes.push_back(TaskType);
	}

	return !Error;
}


CParserCache::CacheType CParserCache::m_Cached;

CParser& CParserCache::Get(const char* str)
{
	CacheType::iterator it = m_Cached.find(str);
	if (it == m_Cached.end())
	{
		CParser* parser = new CParser;
		parser->InputTaskType("", str);
		m_Cached[str] = parser;
		return *parser;
	}
	else
	{
		CParser* parser = it->second;
		return *parser;
	}
}
