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

/**
 * @author Gustav Larsson
 *
 * Adapter function that catches Xerces Reading Exceptions
 * and lets us output them in Prometheus CLogFile.
 *
 * Used for all Xerces C++ Parser reading.
 *
 * @see http://xml.apache.org/xerces-c/apiDocs/classErrorHandler.html
 */
class CXercesErrorHandler : public XERCES_CPP_NAMESPACE::ErrorHandler
{
public:
    CXercesErrorHandler() :
       fSawErrors(false)
    {}

    ~CXercesErrorHandler()
    {}

    // -----------------------------------------------------------------------
    /** @name Implementation of the error handler interface */
    // -----------------------------------------------------------------------
	//@{
	/**
	 * Sends warning exceptions here.
	 */
	void warning(const XERCES_CPP_NAMESPACE::SAXParseException& toCatch);

	/**
	 * Sends error exceptions here.
	 */
	void error(const XERCES_CPP_NAMESPACE::SAXParseException& toCatch);

	/**
	 * Sends fatal error exceptions here.
	 */
	void fatalError(const XERCES_CPP_NAMESPACE::SAXParseException& toCatch);

	/**
	 * Sets fSawError to false.
	 */
    void resetErrors();

	//@}
    // -----------------------------------------------------------------------
    /** @name Access Functions */
    // -----------------------------------------------------------------------
	//@{
	/**
	 * @return true if Errors Occured
	 */
	bool getSawErrors() const { return fSawErrors; }

	//@}
private:
    // -----------------------------------------------------------------------
    /** @name  Private data members */
    // -----------------------------------------------------------------------
	//@{

	/**
	 * This is set if we get any errors, and is queryable via an access
     * function. Its used by the main code to suppress output if there are
     * errors.
	 *
	 * @see getSawErrors()
	 */
    bool fSawErrors;

	//@}
};

#endif