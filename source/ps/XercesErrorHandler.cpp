/*
Xerces Error Handler for Pyrogenesis (and the GUI)
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
#include "Pyrogenesis.h"
#include "CLogger.h"

#define LOG_CATEGORY "xml"

// Use namespace
XERCES_CPP_NAMESPACE_USE

void CXercesErrorHandler::warning(const SAXParseException &toCatch)
{
	CStr systemId=XMLTranscode(toCatch.getSystemId());
	CStr message=XMLTranscode(toCatch.getMessage());
	
	LOG(WARNING, LOG_CATEGORY, "XML Parse Warning: %s:%d:%d: %s",
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

	LOG(ERROR, LOG_CATEGORY, "XML Parse Error: %s:%d:%d: %s",
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

	LOG(ERROR, LOG_CATEGORY, "XML Parse Error (Fatal): %s:%d:%d: %s",
		systemId.c_str(),
		toCatch.getLineNumber(),
		toCatch.getColumnNumber(),
		message.c_str());
}

void CXercesErrorHandler::resetErrors()
{
	fSawErrors = false;
}
