/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
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
 * $Log: XSNamedMap.hpp,v $
 * Revision 1.3  2004/01/29 11:46:30  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.2  2003/11/06 15:30:04  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.1  2003/09/16 14:33:36  neilg
 * PSVI/schema component model classes, with Makefile/configuration changes necessary to build them
 *
 */


#if !defined(XSNAMEDMAP_HPP)
#define XSNAMEDMAP_HPP


#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/RefHash2KeysTableOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLStringPool;

/*
 * This template provides convenient mappings between name,namespace
 * pairs and individual components, as well as means to iterate through all the 
 * named components on some object.
 */

template <class TVal> class XSNamedMap: public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    XSNamedMap(const unsigned int maxElems,
        const unsigned int modulus,
        XMLStringPool* uriStringPool,
        const bool adoptElems,
        MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    //@}

    /** @name Destructor */
    //@{
    ~XSNamedMap();

    //@}

    // -----------------------------------------------------------------------
    //  XSNamedMap methods
    // -----------------------------------------------------------------------
    /** @name XSNamedMap methods */ 
    //@{

    /**
     * The number of <code>XSObjects</code> in the <code>XSObjectList</code>. 
     * The range of valid child object indices is 0 to 
     * <code>mapLength-1</code> inclusive. 
     */
    unsigned int getLength();

    /**
     *  Returns the <code>index</code>th item in the collection. The index 
     * starts at 0. If <code>index</code> is greater than or equal to the 
     * number of objects in the list, this returns <code>null</code>. 
     * @param index  index into the collection. 
     * @return  The <code>XSObject</code> at the <code>index</code>th 
     *   position in the <code>XSObjectList</code>, or <code>null</code> if 
     *   that is not a valid index. 
     */
    TVal *item(unsigned int index);

    /**
     * Retrieves a component specified by local name and namespace URI.
     * <br>applications must use the value null as the 
     * <code>compNamespace</code> parameter for components whose targetNamespace property
     * is absent.
     * @param compNamespace The namespace URI of the component to retrieve.
     * @param localName The local name of the component to retrieve.
     * @return A component (of any type) with the specified local 
     *   name and namespace URI, or <code>null</code> if they do not 
     *   identify any node in this map.
     */
    TVal *itemByName(const XMLCh *compNamespace, 
                              const XMLCh *localName);

    //@}

    //----------------------------------
    /** methods needed by implementation */

    //@{
    void addElement(TVal* const toAdd, const XMLCh* key1, const XMLCh* key2);
    //@}
    

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XSNamedMap(const XSNamedMap<TVal>&);
    XSNamedMap<TVal>& operator=(const XSNamedMap<TVal>&);

    // -----------------------------------------------------------------------
    //  Data members
    //
    // fMemoryManager
    //  manager used to allocate memory needed by this object
    MemoryManager *const        fMemoryManager;
    XMLStringPool*              fURIStringPool;
    RefVectorOf<TVal>*          fVector;
    RefHash2KeysTableOf<TVal>*  fHash;
};



XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/framework/psvi/XSNamedMap.c>
#endif

#endif
