/*
Xerces Error Handler for Prometheus (and the GUI)
by Gustav Larsson
gee@pyro.nu
*/

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------

#include "precompiled.h"

#include "XercesErrorHandler.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "Prometheus.h"
#include "CLogger.h"

// Use namespace
XERCES_CPP_NAMESPACE_USE

void CXercesErrorHandler::warning(const SAXParseException &toCatch)
{
	CStr systemId=XMLTranscode(toCatch.getSystemId());
	CStr message=XMLTranscode(toCatch.getMessage());
	
	LOG(WARNING, "XML Parse Warning: %s:%d:%d: %s",
		systemId.c_str(),
		toCatch.getLineNumber(),
		toCatch.getColumnNumber(),
		message.c_str());
}

void CXercesErrorHandler::error(const SAXParseException& toCatch)
{
	CStr systemId=XMLTranscode(toCatch.getSystemId());
	CStr message=XMLTranscode(toCatch.getMessage());
	fSawErrors = true;

	LOG(ERROR, "XML Parse Error: %s:%d:%d: %s",
		systemId.c_str(),
		toCatch.getLineNumber(),
		toCatch.getColumnNumber(),
		message.c_str());
}

void CXercesErrorHandler::fatalError(const SAXParseException& toCatch)
{
	CStr systemId=XMLTranscode(toCatch.getSystemId());
	CStr message=XMLTranscode(toCatch.getMessage());
	fSawErrors = true;

	LOG(ERROR, "XML Parse Error (Fatal): %s:%d:%d: %s",
		systemId.c_str(),
		toCatch.getLineNumber(),
		toCatch.getColumnNumber(),
		message.c_str());
}

void CXercesErrorHandler::resetErrors()
{
	fSawErrors = false;
}
