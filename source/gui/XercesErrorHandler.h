/*
Xerces Error Handler for Prometheus (and the GUI)
by Gustav Larsson
gee@pyro.nu

--Overview--

	This is a class that that will let us output
	 Xerces C++ Parser errors in our own Log
	 or whatever, fit to Prometheus and foremost
	 the GUI.

--More info--

	http://xml.apache.org/xerces-c/apiDocs/classErrorHandler.html

*/


#ifndef XercesErrorHandler_H
#define XercesErrorHandler_H

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <iostream>


class CXercesErrorHandler : public XERCES_CPP_NAMESPACE::ErrorHandler
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    CXercesErrorHandler() :
       fSawErrors(false)
    {
    }

    ~CXercesErrorHandler()
    {
    }


    // -----------------------------------------------------------------------
    //  Implementation of the error handler interface
    // -----------------------------------------------------------------------
	void warning(const XERCES_CPP_NAMESPACE::SAXParseException& toCatch);
	void error(const XERCES_CPP_NAMESPACE::SAXParseException& toCatch);
	void fatalError(const XERCES_CPP_NAMESPACE::SAXParseException& toCatch);
    void resetErrors();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    bool getSawErrors() const;

private:
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fSawErrors
    //      This is set if we get any errors, and is queryable via a getter
    //      method. Its used by the main code to suppress output if there are
    //      errors.
    // -----------------------------------------------------------------------
    bool fSawErrors;
};

inline bool CXercesErrorHandler::getSawErrors() const
{
    return fSawErrors;
}

#endif