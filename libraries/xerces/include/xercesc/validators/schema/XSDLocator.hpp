/*
 * Copyright 2002,2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: XSDLocator.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */


/**
  * A Locator implementation
  */

#ifndef XSDLOCATOR_HPP
#define XSDLOCATOR_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/sax/Locator.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT XSDLocator: public XMemory, public Locator
{
public:

    /** @name Constructors and Destructor */
    //@{
    /** Default constructor */
    XSDLocator();

    /** Destructor */
    virtual ~XSDLocator()
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
    virtual const XMLCh* getPublicId() const;

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
    virtual const XMLCh* getSystemId() const;

  /**
    * Return the line number where the current document event ends.
    * Note that this is the line position of the first character
    * after the text associated with the document event.
    * @return The line number, or -1 if none is available.
    * @see #getColumnNumber
    */
    virtual XMLSSize_t getLineNumber() const;

  /**
    * Return the column number where the current document event ends.
    * Note that this is the column number of the first
    * character after the text associated with the document
    * event.  The first column in a line is position 1.
    * @return The column number, or -1 if none is available.
    * @see #getLineNumber
    */
    virtual XMLSSize_t getColumnNumber() const;
    //@}

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setValues(const XMLCh* const systemId,
                   const XMLCh* const publicId,
                   const XMLSSize_t lineNo, const XMLSSize_t columnNo);

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and destructor
    // -----------------------------------------------------------------------
    XSDLocator(const XSDLocator&);
    XSDLocator& operator=(const XSDLocator&);

    // -----------------------------------------------------------------------
    //  Private data members
    // -----------------------------------------------------------------------
    XMLSSize_t fLineNo;
    XMLSSize_t fColumnNo;
    const XMLCh* fSystemId;
    const XMLCh* fPublicId;
};

// ---------------------------------------------------------------------------
//  XSDLocator: Getter methods
// ---------------------------------------------------------------------------
inline XMLSSize_t XSDLocator::getLineNumber() const
{
    return fLineNo;
}

inline XMLSSize_t XSDLocator::getColumnNumber() const
{
    return fColumnNo;
}

inline const XMLCh* XSDLocator::getPublicId() const
{
    return fPublicId;
}

inline const XMLCh* XSDLocator::getSystemId() const
{
    return fSystemId;
}

XERCES_CPP_NAMESPACE_END

#endif
