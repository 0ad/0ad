/*
Xerces Error Handler for Prometheus (and the GUI)
by Gustav Larsson
gee@pyro.nu
*/

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/sax/SAXParseException.hpp>
#include "XercesErrorHandler.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>

// Use namespace
XERCES_CPP_NAMESPACE_USE


void XercesErrorHandler::warning(const SAXParseException&)
{
	//
	// Ignore all warnings.
	//
}

void XercesErrorHandler::error(const SAXParseException& toCatch)
{
	char * buf = XMLString::transcode(toCatch.getMessage());
	fSawErrors = true;

        XMLString::release(&buf);
/*    cerr << "Error at file \"" << StrX(toCatch.getSystemId())
		 << "\", line " << toCatch.getLineNumber()
		 << ", column " << toCatch.getColumnNumber()
         << "\n   Message: " << StrX(toCatch.getMessage()) << endl;

*/	
///	g_nemLog(" Error: %s", XMLString::transcode(toCatch.getMessage()));
}

void XercesErrorHandler::fatalError(const SAXParseException& toCatch)
{
	char * buf = XMLString::transcode(toCatch.getMessage());
	fSawErrors = true;


        XMLString::release(&buf);

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
