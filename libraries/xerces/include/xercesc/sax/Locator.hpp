/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xerces" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache\@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation, and was
 * originally based on software copyright (c) 1999, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Log: Locator.hpp,v $
 * Revision 1.4  2003/03/07 18:10:06  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.3  2002/11/04 14:56:26  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/05/27 18:33:07  tng
 * To get ready for 64 bit large file, use XMLSSize_t to represent line and column number.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:08  peiyongz
 * sane_include
 *
 * Revision 1.6  2000/03/02 19:54:35  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.5  2000/02/24 20:12:55  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.4  2000/02/12 03:31:55  rahulj
 * Removed duplicate CVS Log entries.
 *
 * Revision 1.3  2000/02/09 01:55:06  abagchi
 * Removed private function docs
 *
 * Revision 1.2  2000/02/06 07:47:58  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:07:46  twl
 * Initial checkin
 *
 * Revision 1.3  1999/11/08 20:45:01  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#ifndef LOCATOR_HPP
#define LOCATOR_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * Interface for associating a SAX event with a document location.
  *
  * <p>If a SAX parser provides location information to the SAX
  * application, it does so by implementing this interface and then
  * passing an instance to the application using the document
  * handler's setDocumentLocator method.  The application can use the
  * object to obtain the location of any other document handler event
  * in the XML source document.</p>
  *
  * <p>Note that the results returned by the object will be valid only
  * during the scope of each document handler method: the application
  * will receive unpredictable results if it attempts to use the
  * locator at any other time.</p>
  *
  * <p>SAX parsers are not required to supply a locator, but they are
  * very strong encouraged to do so.  If the parser supplies a
  * locator, it must do so before reporting any other document events.
  * If no locator has been set by the time the application receives
  * the startDocument event, the application should assume that a
  * locator is not available.</p>
  *
  * @see DocumentHandler#setDocumentLocator
  */

class SAX_EXPORT Locator
{
public:

    /** @name Constructors and Destructor */
    //@{
    /** Default constructor */
    Locator()
    {
    }

    /** Destructor */
    virtual ~Locator()
    {
    }

    //@}

    /** @name The locator interface */
    //@{
  /**
    * Return the public identifier for the current document event.
    * <p>This will be the public identifier
    * @return A string containing the public identifier, or
    *         null if none is available.
    * @see #getSystemId
    */
    virtual const XMLCh* getPublicId() const = 0;

  /**
    * Return the system identifier for the current document event.
    *
    * <p>If the system identifier is a URL, the parser must resolve it
    * fully before passing it to the application.</p>
    *
    * @return A string containing the system identifier, or null
    *         if none is available.
    * @see #getPublicId
    */
    virtual const XMLCh* getSystemId() const = 0;

  /**
    * Return the line number where the current document event ends.
    * Note that this is the line position of the first character
    * after the text associated with the document event.
    * @return The line number, or -1 if none is available.
    * @see #getColumnNumber
    */
    virtual XMLSSize_t getLineNumber() const = 0;

  /**
    * Return the column number where the current document event ends.
    * Note that this is the column number of the first
    * character after the text associated with the document
    * event.  The first column in a line is position 1.
    * @return The column number, or -1 if none is available.
    * @see #getLineNumber
    */
    virtual XMLSSize_t getColumnNumber() const = 0;
    //@}

private :
    /* Copy constructor */
    Locator(const Locator&);

    /* Assignment operator */
    Locator& operator=(const Locator&);
};

XERCES_CPP_NAMESPACE_END

#endif
