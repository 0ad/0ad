//////////////////////////////////////////////////////////////////////////////
//    AUTHOR: Michael Reiland
//  FILENAME: Error.h
//   PURPOSE: Provides a simple Error Mechanism
//
//    USEAGE: 
//      SetError_Long(PS_FAIL,"A Sample Error",__FILE__,__LINE__);
//      SetError_Short(PS_FAIL,"A Sample Error");
//      TestError(PS_FAIL);
//
//      TODO: Figure out how we're going to clean up and exit upon failure of
//            SetError()
//
//  MODIFIED: 07.20.2003 mreiland

#ifndef _ERROR_H_
#define _ERROR_H_


#include "Singleton.h"
#include "Pyrogenesis.h"

#include <string>


// Used to specify Null pointers.
const int NullPtr = 0;



// Setup error interface
#define IsError() GError.ErrorMechanism_Error()
#define ClearError() GError.ErrorMechanism_ClearError()
#define TestError(TError)  GError.ErrorMechanism_TestError(TError)

#define SetError_Short(w,x) GError.ErrorMechanism_SetError(w,x,__FILE__,__LINE__)
#define SetError_Long(w,x,y,z) GError.ErrorMechanism_SetError(w,x,y,z)

#define GetError() GError.ErrorMechanism_GetError()




/****************************************************************************/
//                        USER DEFINED TYPES                                //
/****************************************************************************/


//////////////////////////////////////////////////////////////////////////////
//
//    NAME: CError
// PURPOSE: Encapsulation of the data for our error framework.
//
class CError
{
public:

	PS_RESULT ErrorName;		// Actual name of the error
	std::string Description;	// Description of the error
	std::string FileName;		// File where the error occured
	unsigned int LineNumber;	// Line where the file occured

	CError()
		:ErrorName(NullPtr), LineNumber(-1){;}
};
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//
//    NAME: ErrorMechanism
// PURPOSE: Provides the interface for the Error mechanism we are using.  It's
//           implemented as a singleton so that only one framework is in
//           existence at one time.
//
class ErrorMechanism : Singleton<ErrorMechanism>
{
	public:

	inline bool ErrorMechanism_Error();
	inline void ErrorMechanism_ClearError();
	inline bool ErrorMechanism_TestError( PS_RESULT);

	inline void ErrorMechanism_SetError(
		PS_RESULT, 
		std::string, 
		std::string, 
		unsigned int);

	inline CError ErrorMechanism_GetError();

private:
	CError ErrorVar;

};
//////////////////////////////////////////////////////////////////////////////




/******************************************************************************/
//                        FUNCTION DEFINITIONS                                //
/******************************************************************************/




//////////////////////////////////////////////////////////////////////////////
//
// Tests if an error exists.  If so, returns true
//
bool ErrorMechanism::ErrorMechanism_Error()
{
	return ( ErrorVar.ErrorName != NullPtr );
}


//////////////////////////////////////////////////////////////////////////////
//
// The error has been dealt with, and needs to be cleared.
//
void ErrorMechanism::ErrorMechanism_ClearError()
{
	ErrorVar.ErrorName = NullPtr;
}


//////////////////////////////////////////////////////////////////////////////
//
// Checks  if the error passed in is the error that's occurred
//
bool ErrorMechanism::ErrorMechanism_TestError( PS_RESULT TErrorName)
{
	return ( ErrorVar.ErrorName == TErrorName );
}


//////////////////////////////////////////////////////////////////////////////
//
// Set's an error that's just occured.
//
void ErrorMechanism::ErrorMechanism_SetError( PS_RESULT TErrorName, std::string TDescription,
					 std::string TFileName,  unsigned int TLineNumber)
{

	if( ErrorMechanism::ErrorVar.ErrorName == NullPtr )
	{
		ErrorVar.ErrorName = TErrorName;
		ErrorVar.Description = TDescription;
		ErrorVar.FileName = TFileName;
		ErrorVar.LineNumber = TLineNumber;
	}
	else
	{
		// We need to clean up properly and exit.
		exit(EXIT_FAILURE);
	}
}


//////////////////////////////////////////////////////////////////////////////
//
// Getter for the error
//
CError ErrorMechanism::ErrorMechanism_GetError()
{
	return ErrorVar;
}

#endif
