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
 * $Log: PSVIAttributeList.hpp,v $
 * Revision 1.7  2004/02/15 19:37:16  amassari
 * Removed cause for warnings in VC 7.1
 *
 * Revision 1.6  2003/12/20 06:19:38  neilg
 * store name/namespace of corresponding attribute in PSVIAttributeList; not all PSVIAttributes have XSAttributeDeclarations
 *
 * Revision 1.5  2003/12/15 17:23:48  cargilld
 * psvi updates; cleanup revisits and bug fixes
 *
 * Revision 1.4  2003/12/02 16:21:41  neilg
 * fix for ArrayIndexOutOfBoundsException in PSVIAttributeList; thanks to Pete Lloyd
 *
 * Revision 1.3  2003/11/26 16:20:37  neilg
 * more complete implementation of PSVIAttributeList; remove some problematic const-ness
 *
 * Revision 1.2  2003/11/06 15:30:04  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.1  2003/09/16 14:33:36  neilg
 * PSVI/schema component model classes, with Makefile/configuration changes necessary to build them
 *
 */

#if !defined(PSVIATTRIBUTEDERIVATION_LIST_HPP)
#define PSVIATTRIBUTEDERIVATION_LIST_HPP

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/psvi/PSVIAttribute.hpp>
#include <xercesc/util/ValueVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * A container for the PSVI contributions to attributes that occur
 * on a particular element.
 * This is always owned by the parser/validator from
 * which it is obtained.  The parser/validator will specify 
 * under what conditions it may be relied upon to have meaningful contents.
 */


class XMLPARSER_EXPORT PSVIAttributeList : public XMemory
{
public:

    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    /**
      * The default constructor 
      *
      * @param  manager     The configurable memory manager
      */
    PSVIAttributeList( MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    //@};

    /** @name Destructor */
    //@{
    ~PSVIAttributeList();
    //@}

    //---------------------
    /** @name PSVIAttributeList methods */

    //@{

    /*
     * Get the number of attributes whose PSVI contributions
     * are contained in this list.
     */
    unsigned int getLength() const;

    /*
     * Get the PSVI contribution of attribute at position i
     * in this list.  Indeces start from 0.
     * @param index index from which the attribute PSVI contribution
     * is to come.  
     * @return PSVIAttribute containing the attributes PSVI contributions;
     * null is returned if the index is out of range.
     */
    PSVIAttribute *getAttributePSVIAtIndex(const unsigned int index);

    /*
     * Get local part of attribute name at position index in the list.
     * Indeces start from 0.
     * @param index index from which the attribute name 
     * is to come.  
     * @return local part of the attribute's name; null is returned if the index
     * is out of range.
     */
    const XMLCh *getAttributeNameAtIndex(const unsigned int index);

    /*
     * Get namespace of attribute at position index in the list.
     * Indeces start from 0.
     * @param index index from which the attribute namespace 
     * is to come.  
     * @return namespace of the attribute; 
     * null is returned if the index is out of range.
     */
    const XMLCh *getAttributeNamespaceAtIndex(const unsigned int index);

    /*
     * Get the PSVI contribution of attribute with given 
     * local name and namespace.
     * @param attrName  local part of the attribute's name
     * @param attrNamespace  namespace of the attribute
     * @return null if the attribute PSVI does not exist
     */
    PSVIAttribute *getAttributePSVIByName(const XMLCh *attrName
                    , const XMLCh * attrNamespace);

    //@}

    //----------------------------------
    /** methods needed by implementation */

    //@{

    /**
      * returns a PSVI attribute of undetermined state and given name/namespace and 
      * makes that object part of the internal list.  Intended to be called
      * during validation of an element.
      * @param attrName     name of this attribute
      * @param attrNS       URI of the attribute
      * @return             new, uninitialized, PSVIAttribute object
      */
    PSVIAttribute *getPSVIAttributeToFill(
            const XMLCh * attrName
            , const XMLCh * attrNS);

    /**
      * reset the list
      */
    void reset();

    //@}

private:

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    PSVIAttributeList(const PSVIAttributeList&);
    PSVIAttributeList & operator=(const PSVIAttributeList &);


    // -----------------------------------------------------------------------
    //  data members
    // -----------------------------------------------------------------------
    // fMemoryManager
    //  handler to provide dynamically-need memory
    // fAttrList
    //  list of PSVIAttributes contained by this object
    // fAttrNameList
    //  list of the names of the initialized PSVIAttribute objects contained
    //  in this listing
    // fAttrNSList
    //  list of the namespaces of the initialized PSVIAttribute objects contained
    //  in this listing
    // fAttrPos
    //  current number of initialized PSVIAttributes in fAttrList
    MemoryManager*                  fMemoryManager;    
    RefVectorOf<PSVIAttribute>*     fAttrList;
    RefArrayVectorOf<XMLCh>*        fAttrNameList;
    RefArrayVectorOf<XMLCh>*        fAttrNSList;
    unsigned int                    fAttrPos;
};
inline PSVIAttributeList::~PSVIAttributeList() 
{
    delete fAttrList;
    delete fAttrNameList;
    delete fAttrNSList;
}

inline PSVIAttribute *PSVIAttributeList::getPSVIAttributeToFill(
            const XMLCh *attrName
            , const XMLCh * attrNS)
{
    PSVIAttribute *retAttr = 0;
    if(fAttrPos == fAttrList->size())
    {
        retAttr = new (fMemoryManager)PSVIAttribute(fMemoryManager);
        fAttrList->addElement(retAttr);
        fAttrNameList->addElement((XMLCh *)attrName);
        fAttrNSList->addElement((XMLCh *)attrNS);
    }
    else
    {
        retAttr = fAttrList->elementAt(fAttrPos);
        fAttrNameList->setElementAt((XMLCh *)attrName, fAttrPos);
        fAttrNSList->setElementAt((XMLCh *)attrNS, fAttrPos);
    }
    fAttrPos++;
    return retAttr;
}

inline void PSVIAttributeList::reset()
{
    fAttrPos = 0;
}

XERCES_CPP_NAMESPACE_END

#endif
