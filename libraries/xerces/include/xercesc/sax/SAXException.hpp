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
 * $Log: SAXException.hpp,v $
 * Revision 1.6  2003/12/01 23:23:26  neilg
 * fix for bug 25118; thanks to Jeroen Witmond
 *
 * Revision 1.5  2003/08/13 15:43:24  knoaman
 * Use memory manager when creating SAX exceptions.
 *
 * Revision 1.4  2003/05/15 18:27:05  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2002/12/06 13:17:29  tng
 * [Bug 9083] Make SAXNotSupportedException and SAXNotRecognizedException to be exportable
 *
 * Revision 1.2  2002/11/04 14:56:26  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:08  peiyongz
 * sane_include
 *
 * Revision 1.8  2000/09/07 23:55:02  andyh
 * Fix SAXException assignment operator.  Now non-virtual, and
 * SAXParseException invokes base class operator.
 *
 * Revision 1.7  2000/08/09 22:06:04  jpolast
 * more functionality to SAXException and its children.
 * msgs are now functional for SAXNotSupportedEx and
 * SAXNotRecognizedEx
 *
 * Revision 1.6  2000/08/02 18:04:02  jpolast
 * include SAXNotSupportedException and
 * SAXNotRecognizedException needed for sax2
 *
 * Revision 1.5  2000/02/24 20:12:55  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.4  2000/02/09 19:15:17  abagchi
 * Inserted documentation for new APIs
 *
 * Revision 1.3  2000/02/06 07:47:58  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.2  1999/12/18 00:21:23  roddey
 * Fixed a small reported memory leak
 *
 * Revision 1.1.1.1  1999/11/09 01:07:47  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:02  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#ifndef SAXEXCEPTION_HPP
#define SAXEXCEPTION_HPP

#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMemory.hpp>

XERCES_CPP_NAMESPACE_BEGIN


/**
  * Encapsulate a general SAX error or warning.
  *
  * <p>This class can contain basic error or warning information from
  * either the XML SAX parser or the application: a parser writer or
  * application writer can subclass it to provide additional
  * functionality.  SAX handlers may throw this exception or
  * any exception subclassed from it.</p>
  *
  * <p>If the application needs to pass through other types of
  * exceptions, it must wrap those exceptions in a SAXException
  * or an exception derived from a SAXException.</p>
  *
  * <p>If the parser or application needs to include information
  * about a specific location in an XML document, it should use the
  * SAXParseException subclass.</p>
  *
  * @see SAXParseException#SAXParseException
  */
class SAX_EXPORT SAXException : public XMemory
{
public:
    /** @name Constructors and Destructor */
    //@{
    /** Default constructor
     * @param manager    Pointer to the memory manager to be used to
     *                   allocate objects.
     */
    SAXException(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) :

        fMsg(XMLString::replicate(XMLUni::fgZeroLenString, manager))
        , fMemoryManager(manager)
    {
    }

  /**
    * Create a new SAXException.
    *
    * @param msg The error or warning message.
    * @param manager    Pointer to the memory manager to be used to
    *                   allocate objects.
    */
    SAXException(const XMLCh* const msg,
                 MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) :

        fMsg(XMLString::replicate(msg, manager))
        , fMemoryManager(manager)
    {
    }

  /**
    * Create a new SAXException.
    *
    * @param msg The error or warning message.
    * @param manager    Pointer to the memory manager to be used to
    *                   allocate objects.
    */
    SAXException(const char* const msg,
                 MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) :

        fMsg(XMLString::transcode(msg, manager))
        , fMemoryManager(manager)
    {
    }

  /**
    * Copy constructor
    *
    * @param toCopy The exception to be copy constructed
    */
    SAXException(const SAXException& toCopy) :

        fMsg(XMLString::replicate(toCopy.fMsg, toCopy.fMemoryManager))
        , fMemoryManager(toCopy.fMemoryManager)
    {
    }

    /** Destructor */
    virtual ~SAXException()
    {
        fMemoryManager->deallocate(fMsg);//delete [] fMsg;
    }

    //@}


    /** @name Public Operators */
    //@{
    /**
      * Assignment operator
      *
      * @param toCopy The object to be copied
      */
    SAXException& operator=(const SAXException& toCopy)
    {
        if (this == &toCopy)
            return *this;

        fMemoryManager->deallocate(fMsg);//delete [] fMsg;
        fMsg = XMLString::replicate(toCopy.fMsg, toCopy.fMemoryManager);
        fMemoryManager = toCopy.fMemoryManager;
        return *this;
    }
    //@}

    /** @name Getter Methods */
    //@{
    /**
      * Get the contents of the message
      *
      */
    virtual const XMLCh* getMessage() const
    {
        return fMsg;
    }
    //@}


protected :
    // -----------------------------------------------------------------------
    //  Protected data members
    //
    //  fMsg
    //      This is the text of the error that is being thrown.
    // -----------------------------------------------------------------------
    XMLCh*  fMsg;
    MemoryManager* fMemoryManager;
};

class SAX_EXPORT SAXNotSupportedException : public SAXException
{

public:
	SAXNotSupportedException(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

  /**
    * Create a new SAXException.
    *
    * @param msg The error or warning message.
    * @param manager    Pointer to the memory manager to be used to
    *                   allocate objects.
    */
    SAXNotSupportedException(const XMLCh* const msg,
                             MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

  /**
    * Create a new SAXException.
    *
    * @param msg The error or warning message.
    * @param manager    Pointer to the memory manager to be used to
    *                   allocate objects.
    */
    SAXNotSupportedException(const char* const msg,
                             MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

  /**
    * Copy constructor
    *
    * @param toCopy The exception to be copy constructed
    */
    SAXNotSupportedException(const SAXException& toCopy);
};

class SAX_EXPORT SAXNotRecognizedException : public SAXException
{
public:
	SAXNotRecognizedException(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

  /**
    * Create a new SAXException.
    *
    * @param msg The error or warning message.
    * @param manager    Pointer to the memory manager to be used to
    *                   allocate objects.
    */
    SAXNotRecognizedException(const XMLCh* const msg,
                              MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

  /**
    * Create a new SAXException.
    *
    * @param msg The error or warning message.
    * @param manager    Pointer to the memory manager to be used to
    *                   allocate objects.
    */
    SAXNotRecognizedException(const char* const msg,
                              MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

  /**
    * Copy constructor
    *
    * @param toCopy The exception to be copy constructed
    */
    SAXNotRecognizedException(const SAXException& toCopy);
};

XERCES_CPP_NAMESPACE_END

#endif
