/*
Xerces Error Handler for Prometheus (and the GUI)
by Gustav Larsson
gee@pyro.nu
*/

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------

#include "precompiled.h"

#include <xercesc/sax/SAXParseException.hpp>
#include "XercesErrorHandler.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "Prometheus.h"
#include "CLogger.h"

// Use namespace
XERCES_CPP_NAMESPACE_USE


void CXercesErrorHandler::warning(const SAXParseException&)
{
	//
	// Ignore all warnings.
	//
}

void CXercesErrorHandler::error(const SAXParseException& toCatch)
{
	char * buf = XMLString::transcode(toCatch.getMessage());
	fSawErrors = true;

        XMLString::release(&buf);
/*    cerr << "Error at file \"" << StrX(toCatch.getSystemId())
		 << "\", line " << toCatch.getLineNumber()
		 << ", column " << toCatch.getColumnNumber()
         << "\n   Message: " << StrX(toCatch.getMessage()) << endl;

*/
	LOG(ERROR, "XML Parse Error: %s:%d:%d: %s",
		XMLString::transcode(toCatch.getSystemId()),
		toCatch.getLineNumber(),
		toCatch.getColumnNumber(),
		XMLString::transcode(toCatch.getMessage()));
///	g_nemLog(" Error: %s", XMLString::transcode(toCatch.getMessage()));
}

void CXercesErrorHandler::fatalError(const SAXParseException& toCatch)
{
	char * buf = XMLString::transcode(toCatch.getMessage());
	fSawErrors = true;


        XMLString::release(&buf);

/*    cerr << "Fatal Error at file \"" << StrX(toCatch.getSystemId())
		 << "\", line " << toCatch.getLineNumber()
		 << ", column " << toCatch.getColumnNumber()
         << "\n   Message: " << StrX(toCatch.getMessage()) << endl;
*/
	LOG(ERROR, "XML Parse Error: %s:%d:%d: %s",
		XMLString::transcode(toCatch.getSystemId()),
		toCatch.getLineNumber(),
		toCatch.getColumnNumber(),
		XMLString::transcode(toCatch.getMessage()));
///	g_nemLog(" Error: %s", XMLString::transcode(toCatch.getMessage()));
}

void CXercesErrorHandler::resetErrors()
{
	fSawErrors = false;
}
