/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2003 The Apache Software Foundation.  All rights
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
 * $Log: XMLNotationDecl.hpp,v $
 * Revision 1.12  2004/02/15 19:37:16  amassari
 * Removed cause for warnings in VC 7.1
 *
 * Revision 1.11  2003/11/21 22:34:46  neilg
 * More schema component model implementation, thanks to David Cargill.
 * In particular, this cleans up and completes the XSModel, XSNamespaceItem,
 * XSAttributeDeclaration and XSAttributeGroup implementations.
 *
 * Revision 1.10  2003/11/06 15:30:06  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.9  2003/10/10 16:23:29  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.8  2003/05/22 02:10:51  knoaman
 * Default the memory manager.
 *
 * Revision 1.7  2003/05/16 21:36:55  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.6  2003/05/15 18:26:07  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2003/04/21 20:46:01  knoaman
 * Use XMLString::release to prepare for configurable memory manager.
 *
 * Revision 1.4  2003/03/07 18:08:10  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.3  2002/11/04 15:00:21  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/08/22 19:27:41  tng
 * [Bug 11448] DomCount has problems with XHTML1.1 DTD.
 *
 * Revision 1.1.1.1  2002/02/01 22:21:52  peiyongz
 * sane_include
 *
 * Revision 1.5  2000/03/02 19:54:25  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.4  2000/02/24 20:00:23  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.3  2000/02/15 01:21:31  roddey
 * Some initial documentation improvements. More to come...
 *
 * Revision 1.2  2000/02/06 07:47:48  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:08:35  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:44:39  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#if !defined(XMLNOTATIONDECL_HPP)
#define XMLNOTATIONDECL_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 *  This class represents the core information about a notation declaration
 *  that all validators must at least support. Each validator will create a
 *  derivative of this class which adds any information it requires for its
 *  own extra needs.
 *
 *  At this common level, the information supported is the notation name
 *  and the public and sysetm ids indicated in the notation declaration.
 */
class XMLPARSER_EXPORT XMLNotationDecl : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------

    /** @name Constructors */
    //@{
    XMLNotationDecl(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    XMLNotationDecl
    (
        const   XMLCh* const    notName
        , const XMLCh* const    pubId
        , const XMLCh* const    sysId
        , const XMLCh* const    baseURI = 0
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );
    //@}

    /** @name Destructor */
    //@{
    ~XMLNotationDecl();
    //@}


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    unsigned int getId() const;
    const XMLCh* getName() const;
    const XMLCh* getPublicId() const;
    const XMLCh* getSystemId() const;
    const XMLCh* getBaseURI() const;
    unsigned int getNameSpaceId() const;
    MemoryManager* getMemoryManager() const;


    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setId(const unsigned int newId);
    void setName
    (
        const   XMLCh* const    notName
    );
    void setPublicId(const XMLCh* const newId);
    void setSystemId(const XMLCh* const newId);
    void setBaseURI(const XMLCh* const newId);
    void setNameSpaceId(const unsigned int newId);

    // -----------------------------------------------------------------------
    //  Support named collection element semantics
    // -----------------------------------------------------------------------
    const XMLCh* getKey() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLNotationDecl)

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLNotationDecl(const XMLNotationDecl&);
    XMLNotationDecl& operator=(const XMLNotationDecl&);


    // -----------------------------------------------------------------------
    //  XMLNotationDecl: Private helper methods
    // -----------------------------------------------------------------------
    void cleanUp();


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fId
    //      This is the unique id given to this notation decl.
    //
    //  fName
    //      The notation's name, which identifies the type of notation it
    //      applies to.
    //
    //  fPublicId
    //      The text of the notation's public id, if any.
    //
    //  fSystemId
    //      The text of the notation's system id, if any.
    //
    //  fBaseURI
    //      The text of the notation's base URI
    // -----------------------------------------------------------------------
    unsigned int    fId;
	XMLCh*          fName;
    XMLCh*          fPublicId;
    XMLCh*          fSystemId;
    XMLCh*          fBaseURI;
    unsigned int    fNameSpaceId;
    MemoryManager*  fMemoryManager;
};


// -----------------------------------------------------------------------
//  Getter methods
// -----------------------------------------------------------------------
inline unsigned int XMLNotationDecl::getId() const
{
    return fId;
}

inline const XMLCh* XMLNotationDecl::getName() const
{
    return fName;
}

inline unsigned int XMLNotationDecl::getNameSpaceId() const
{
    return fNameSpaceId;
}

inline const XMLCh* XMLNotationDecl::getPublicId() const
{
    return fPublicId;
}

inline const XMLCh* XMLNotationDecl::getSystemId() const
{
    return fSystemId;
}

inline const XMLCh* XMLNotationDecl::getBaseURI() const
{
    return fBaseURI;
}

inline MemoryManager* XMLNotationDecl::getMemoryManager() const
{
    return fMemoryManager;
}

// -----------------------------------------------------------------------
//  Setter methods
// -----------------------------------------------------------------------
inline void XMLNotationDecl::setId(const unsigned int newId)
{
    fId = newId;
}

inline void XMLNotationDecl::setNameSpaceId(const unsigned int newId)
{
    fNameSpaceId = newId;
}

inline void XMLNotationDecl::setPublicId(const XMLCh* const newId)
{
    if (fPublicId)
        fMemoryManager->deallocate(fPublicId);

    fPublicId = XMLString::replicate(newId, fMemoryManager);
}

inline void XMLNotationDecl::setSystemId(const XMLCh* const newId)
{
    if (fSystemId)
        fMemoryManager->deallocate(fSystemId);

    fSystemId = XMLString::replicate(newId, fMemoryManager);
}

inline void XMLNotationDecl::setBaseURI(const XMLCh* const newId)
{
    if (fBaseURI)
        fMemoryManager->deallocate(fBaseURI);

    fBaseURI = XMLString::replicate(newId, fMemoryManager);
}


// ---------------------------------------------------------------------------
//  XMLNotationDecl: Support named pool element semantics
// ---------------------------------------------------------------------------
inline const XMLCh* XMLNotationDecl::getKey() const
{
    return fName;
}

XERCES_CPP_NAMESPACE_END

#endif
