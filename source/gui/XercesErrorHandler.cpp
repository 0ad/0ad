/*
Xerces Error Handler for Prometheus (and the GUI)
by Gustav Larsson
gee@pyro.nu
*/

///#include "nemesis.h"

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/sax/SAXParseException.hpp>
#include "XercesErrorHandler.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>


void XercesErrorHandler::warning(const SAXParseException&)
{
	//
	// Ignore all warnings.
	//
}

void XercesErrorHandler::error(const SAXParseException& toCatch)
{
	fSawErrors = true;
/*    cerr << "Error at file \"" << StrX(toCatch.getSystemId())
		 << "\", line " << toCatch.getLineNumber()
		 << ", column " << toCatch.getColumnNumber()
         << "\n   Message: " << StrX(toCatch.getMessage()) << endl;

*/	
///	g_nemLog(" Error: %s", XMLString::transcode(toCatch.getMessage()));
}

void XercesErrorHandler::fatalError(const SAXParseException& toCatch)
{
	fSawErrors = true;
/*    cerr << "Fatal Error at file \"" << StrX(toCatch.getSystemId())
		 << "\", line " << toCatch.getLineNumber()
		 << ", column " << toCatch.getColumnNumber()
         << "\n   Message: " << StrX(toCatch.getMessage()) << endl;
*/
///	g_nemLog(" Error: %s", XMLString::transcode(toCatch.getMessage()));
}

void XercesErrorHandler::resetErrors()
{
	fSawErrors = false;
}
