/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Log: QName.hpp,v $
 * Revision 1.10  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.9  2003/09/25 15:22:34  peiyongz
 * Implementation of Serialization
 *
 * Revision 1.8  2003/05/16 21:36:59  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.7  2003/05/16 06:01:52  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.6  2003/05/15 19:04:35  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2002/11/04 15:22:04  tng
 * C++ Namespace Support.
 *
 * Revision 1.4  2002/10/30 21:52:00  tng
 * [Bug 13641] compiler-generated copy-constructor for QName doesn't do the right thing.
 *
 * Revision 1.3  2002/09/05 16:06:41  tng
 * [Bug 12232] Make operator to be constant.
 *
 * Revision 1.2  2002/08/20 16:54:25  tng
 * [Bug 6251] Info during compilation.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:11  peiyongz
 * sane_include
 *
 * Revision 1.8  2001/12/06 17:48:36  tng
 * Performance Enhancement.  Added setNPrefix and setNLocalPart methods that allow code to take advantage of the fact that it knows the length of the prefix and local name, when possible.  That can avoid a copy of the prefix into a null-terminated temporary variable before copying into the fPrefix.
 * Also changed the getRawName method so that it would simply return the local part when there is no prefix, instead of allocating another buffer to copy the local part into the fRawName.
 * When there is a prefix, changed the getRawName to copy the prefix and local part into the fRawName using XMLString::moveChars instead of using XMLString::copyString and XMLString::catString.  The catString method has to loop past the prefix portion of the fRawName, which seems like a waste.
 * By Henry Zongaro.
 *
 * Revision 1.7  2001/07/24 18:31:47  knoaman
 * Added support for <group> + extra constraint checking for complexType
 *
 * Revision 1.6  2001/05/11 13:26:28  tng
 * Copyright update.
 *
 * Revision 1.5  2001/04/19 18:17:10  tng
 * Schema: SchemaValidator update, and use QName in Content Model
 *
 * Revision 1.4  2001/03/21 21:56:12  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 * Revision 1.3  2001/02/27 14:48:39  tng
 * Schema: Add CMAny and ContentLeafNameTypeVector, by Pei Yong Zhang
 *
 * Revision 1.2  2001/02/26 21:56:16  tng
 * Schema: QName can also be constructed with rawName.
 *
 * Revision 1.1  2001/02/26 19:44:25  tng
 * Schema: add utility class QName, by Pei Yong Zhang.
 *
 */

#if !defined(QNAME_HPP)
#define QNAME_HPP

#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT QName : public XSerializable, public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Contructors and Destructor
    // -----------------------------------------------------------------------
    /** Default constructor. */
    QName(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /** Constructs a specified qname using prefix, and localpart. */
    QName
    (
          const XMLCh* const   prefix
        , const XMLCh* const   localPart
	    , const unsigned int   uriId
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Constructs a specified qname using rawName. */
    QName
    (
          const XMLCh* const   rawName
	    , const unsigned int   uriId
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Copy constructor. */
    QName(const QName& qname);

    ~QName();

    // -----------------------------------------------------------------------
    //  Getters
    // -----------------------------------------------------------------------
    const XMLCh* getPrefix() const;
    XMLCh* getPrefix();

    const XMLCh* getLocalPart() const;
    XMLCh* getLocalPart();

    unsigned int getURI() const;

    const XMLCh* getRawName() const;
    XMLCh* getRawName();

    MemoryManager* getMemoryManager() const;

    // -----------------------------------------------------------------------
    //  Setters
    // -----------------------------------------------------------------------
    void setName
    (
        const XMLCh* const        prefix
      , const XMLCh* const        localPart
	   , const unsigned int        uriId
    );

    void setName
    (
        const XMLCh* const        rawName
	   , const unsigned int        uriId
    );

    void setPrefix(const XMLCh*) ;
    void setLocalPart(const XMLCh*) ;
    void setNPrefix(const XMLCh*, const unsigned int) ;
    void setNLocalPart(const XMLCh*, const unsigned int) ;
    void setURI(const unsigned int) ;

    void setValues(const QName& qname);

    // -----------------------------------------------------------------------
    //  comparison
    // -----------------------------------------------------------------------
    bool operator==(const QName&) const;

    // -----------------------------------------------------------------------
    //  Misc
    // -----------------------------------------------------------------------
    void cleanUp();

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(QName)

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------    
    QName& operator=(const QName&);

    // -----------------------------------------------------------------------
    //  Private instance variables
    //
    //  We copy the followings from XMLAttr.hpp, but stick to Java version's
    //  naming convention
    //
    //  fPrefix
    //  fPrefixBufSz
    //      The prefix that was applied to this attribute's name, and the
    //      current size of the buffer (minus one for the null.) Prefixes
    //      really don't matter technically but it might be required for
    //      pratical reasons, to recreate the original document for instance.
    //
    //  fLocalPart
    //  fLocalPartBufSz
    //      The base part of the name of the attribute, and the current size
    //      of the buffer (minus one, where the null is.)
    //
    //  fRawName
    //  fRawNameBufSz
    //      This is the QName form of the name, which is faulted in (from the
    //      prefix and name) upon request. The size field indicates the
    //      current size of the buffer (minus one for the null.) It will be
    //      zero until fauled in.
    //
    //  fURIId
    //      The id of the URI that this attribute belongs to.
    // -----------------------------------------------------------------------
    MemoryManager*      fMemoryManager;
    XMLCh*              fPrefix;
    unsigned int        fPrefixBufSz;
    XMLCh*              fLocalPart;
    unsigned int        fLocalPartBufSz;
    XMLCh*              fRawName;
    unsigned int        fRawNameBufSz;
    unsigned int        fURIId;
};

// ---------------------------------------------------------------------------
//  QName: Getter methods
// ---------------------------------------------------------------------------
inline const XMLCh* QName::getPrefix() const
{
	return fPrefix;
}

inline XMLCh* QName::getPrefix()
{
	return fPrefix;
}

inline const XMLCh* QName::getLocalPart() const
{
	return fLocalPart;
}

inline XMLCh* QName::getLocalPart()
{
	return fLocalPart;
}

inline unsigned int QName::getURI() const
{
	return fURIId;
}

inline MemoryManager* QName::getMemoryManager() const
{
    return fMemoryManager;
}

// ---------------------------------------------------------------------------
//  QName: Setter methods
// ---------------------------------------------------------------------------
inline void QName::setURI(const unsigned int uriId)
{
    fURIId = uriId;
}

XERCES_CPP_NAMESPACE_END

#endif
