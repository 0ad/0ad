/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2001 The Apache Software Foundation.  All rights
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
 * $Log: ElemStack.hpp,v $
 * Revision 1.7  2003/10/22 20:22:30  knoaman
 * Prepare for annotation support.
 *
 * Revision 1.6  2003/05/16 21:36:57  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.5  2003/05/15 18:26:29  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.4  2003/03/07 18:08:58  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.3  2002/12/04 02:23:50  knoaman
 * Scanner re-organization.
 *
 * Revision 1.2  2002/11/04 14:58:18  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:21:58  peiyongz
 * sane_include
 *
 * Revision 1.11  2001/12/12 14:29:50  tng
 * Remove obsolete code in ElemStack which can help performance.
 *
 * Revision 1.10  2001/08/07 13:47:47  tng
 * Schema: Fix unmatched end tag for qualified/unqualifed start tag.
 *
 * Revision 1.9  2001/05/28 20:55:19  tng
 * Schema: Store Grammar in ElemStack as well.
 *
 * Revision 1.8  2001/05/11 13:26:16  tng
 * Copyright update.
 *
 * Revision 1.7  2001/05/03 20:34:28  tng
 * Schema: SchemaValidator update
 *
 * Revision 1.6  2001/04/19 18:16:58  tng
 * Schema: SchemaValidator update, and use QName in Content Model
 *
 * Revision 1.5  2000/04/18 23:54:29  roddey
 * Got rid of some foward references to no longer used classes.
 *
 * Revision 1.4  2000/03/02 19:54:28  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.3  2000/02/24 20:18:07  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/06 07:47:52  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:08:06  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:44:42  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#if !defined(ELEMSTACK_HPP)
#define ELEMSTACK_HPP

#include <xercesc/util/StringPool.hpp>
#include <xercesc/util/QName.hpp>
#include <xercesc/util/ValueVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLElementDecl;
class Grammar;

struct PrefMapElem : public XMemory
{
    unsigned int        fPrefId;
    unsigned int        fURIId;
};

//
//  During the scan of content, we have to keep up with the nesting of
//  elements (for validation and wellformedness purposes) and we have to
//  have places to remember namespace (prefix to URI) mappings.
//
//  We only have to keep a stack of the current path down through the tree
//  that we are currently scanning, and keep track of any children of any
//  elements along that path.
//
//  So, this data structure is a stack, which represents the current path
//  through the tree that we've worked our way down to. For each node in
//  the stack, there is an array of element ids that represent the ids of
//  the child elements scanned so far. Upon exit from that element, its
//  array of child elements is validated.
//
//  Since we have the actual XMLElementDecl in the stack nodes, when its time
//  to validate, we just extract the content model from that element decl
//  and validate. All the required data falls easily to hand. Note that we
//  actually have some derivative of XMLElementDecl, which is specific to
//  the validator used, but the abstract API is sufficient for the needs of
//  the scanner.
//
//  Since the namespace support also requires the storage of information on
//  a nested element basis, this structure also holds the namespace info. For
//  each level, the prefixes defined at that level (and the namespaces that
//  they map to) are stored.
//
class XMLPARSER_EXPORT ElemStack : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Class specific data types
    //
    //  These really should be private, but some of the compilers we have to
    //  support are too dumb to deal with that.
    //
    //  PrefMapElem
    //      fURIId is the id of the URI from the validator's URI map. The
    //      fPrefId is the id of the prefix from our own prefix pool. The
    //      namespace stack consists of these elements.
    //
    //  StackElem
    //      fThisElement is the basic element decl for the current element.
    //      The fRowCapacity is how large fChildIds has grown so far.
    //      fChildCount is how many of them are valid right now.
    //
    //      The fMapCapacity is how large fMap has grown so far. fMapCount
    //      is how many of them are valid right now.
    //
    //      Note that we store the reader number we were in when we found the
    //      start tag. We'll use this at the end tag to test for unbalanced
    //      markup in entities.
    //
    //  MapModes
    //      When a prefix is mapped to a namespace id, it matters whether the
    //      QName being mapped is an attribute or name. Attributes are not
    //      affected by an sibling xmlns attributes, whereas elements are
    //      affected by its own xmlns attributes.
    // -----------------------------------------------------------------------
    struct StackElem : public XMemory
    {
        XMLElementDecl*     fThisElement;
        unsigned int        fReaderNum;

        unsigned int        fChildCapacity;
        unsigned int        fChildCount;
        QName**             fChildren;

        PrefMapElem*        fMap;
        unsigned int        fMapCapacity;
        unsigned int        fMapCount;

        bool                fValidationFlag;
        int                 fCurrentScope;
        Grammar*            fCurrentGrammar;
        unsigned int        fCurrentURI;
    };

    enum MapModes
    {
        Mode_Attribute
        , Mode_Element
    };


    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ElemStack(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~ElemStack();


    // -----------------------------------------------------------------------
    //  Stack access
    // -----------------------------------------------------------------------
    unsigned int addLevel();
    unsigned int addLevel(XMLElementDecl* const toSet, const unsigned int readerNum);
    const StackElem* popTop();


    // -----------------------------------------------------------------------
    //  Stack top access
    // -----------------------------------------------------------------------
    unsigned int addChild(QName* const child, const bool toParent);
    const StackElem* topElement() const;
    void setElement(XMLElementDecl* const toSet, const unsigned int readerNum);

    void setValidationFlag(bool validationFlag);
    bool getValidationFlag();

    void setCurrentScope(int currentScope);
    int getCurrentScope();

    void setCurrentGrammar(Grammar* currentGrammar);
    Grammar* getCurrentGrammar();

    void setCurrentURI(unsigned int uri);
    unsigned int getCurrentURI();

    // -----------------------------------------------------------------------
    //  Prefix map methods
    // -----------------------------------------------------------------------
    void addPrefix
    (
        const   XMLCh* const    prefixToAdd
        , const unsigned int    uriId
    );
    unsigned int mapPrefixToURI
    (
        const   XMLCh* const    prefixToMap
        , const MapModes        mode
        ,       bool&           unknown
    )   const;
    ValueVectorOf<PrefMapElem*>* getNamespaceMap() const;
    unsigned int getPrefixId(const XMLCh* const prefix) const;
    const XMLCh* getPrefixForId(unsigned int prefId) const;

    // -----------------------------------------------------------------------
    //  Miscellaneous methods
    // -----------------------------------------------------------------------
    bool isEmpty() const;
    void reset
    (
        const   unsigned int    emptyId
        , const unsigned int    unknownId
        , const unsigned int    xmlId
        , const unsigned int    xmlNSId
    );


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ElemStack(const ElemStack&);
    ElemStack& operator=(const ElemStack&);


    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    void expandMap(StackElem* const toExpand);
    void expandStack();


    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fEmptyNamespaceId
    //      This is the special URI id for the "" namespace, which is magic
    //      because of the xmlns="" operation.
    //
    //  fGlobalPoolId
    //      This is a special URI id that is returned when the namespace
    //      prefix is "" and no one has explicitly mapped that prefix to an
    //      explicit URI (or when they explicitly clear any such mapping,
    //      which they can also do.) And also its prefix pool id, which is
    //      stored here for fast access.
    //
    //  fPrefixPool
    //      This is the prefix pool where prefixes are hashed and given unique
    //      ids. These ids are used to track prefixes in the element stack.
    //
    //  fStack
    //  fStackCapacity
    //  fStackTop
    //      This the stack array. Its an array of pointers to StackElem
    //      structures. The capacity is the current high water mark of the
    //      stack. The top is the current top of stack (i.e. the part of it
    //      being used.)
    //
    //  fUnknownNamespaceId
    //      This is the URI id for the special URI that is assigned to any
    //      prefix which has not been mapped. This lets us keep going after
    //      issuing the error.
    //
    //  fXMLNamespaceId
    //  fXMLPoolId
    //  fXMLNSNamespaceId
    //  fXMLNSPoolId
    //      These are the URI ids for the special URIs that are assigned to
    //      the 'xml' and 'xmlns' namespaces. And also its prefix pool id,
    //      which is stored here for fast access.
    // -----------------------------------------------------------------------
    unsigned int                 fEmptyNamespaceId;
    unsigned int                 fGlobalPoolId;
    XMLStringPool                fPrefixPool;
    StackElem**                  fStack;
    unsigned int                 fStackCapacity;
    unsigned int                 fStackTop;
    unsigned int                 fUnknownNamespaceId;
    unsigned int                 fXMLNamespaceId;
    unsigned int                 fXMLPoolId;
    unsigned int                 fXMLNSNamespaceId;
    unsigned int                 fXMLNSPoolId;
    ValueVectorOf<PrefMapElem*>* fNamespaceMap;
    MemoryManager*               fMemoryManager;
};


class XMLPARSER_EXPORT WFElemStack : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Class specific data types
    //
    //  These really should be private, but some of the compilers we have to
    //  support are too dumb to deal with that.
    //
    //  PrefMapElem
    //      fURIId is the id of the URI from the validator's URI map. The
    //      fPrefId is the id of the prefix from our own prefix pool. The
    //      namespace stack consists of these elements.
    //
    //  StackElem
    //      fThisElement is the basic element decl for the current element.
    //      The fRowCapacity is how large fChildIds has grown so far.
    //      fChildCount is how many of them are valid right now.
    //
    //      The fMapCapacity is how large fMap has grown so far. fMapCount
    //      is how many of them are valid right now.
    //
    //      Note that we store the reader number we were in when we found the
    //      start tag. We'll use this at the end tag to test for unbalanced
    //      markup in entities.
    //
    //  MapModes
    //      When a prefix is mapped to a namespace id, it matters whether the
    //      QName being mapped is an attribute or name. Attributes are not
    //      affected by an sibling xmlns attributes, whereas elements are
    //      affected by its own xmlns attributes.
    // -----------------------------------------------------------------------
    struct StackElem : public XMemory
    {
        int                 fTopPrefix;        
        unsigned int        fCurrentURI;
        unsigned int        fReaderNum;
        unsigned int        fElemMaxLength;
        XMLCh*              fThisElement;
    };

    enum MapModes
    {
        Mode_Attribute
        , Mode_Element
    };


    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    WFElemStack(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~WFElemStack();


    // -----------------------------------------------------------------------
    //  Stack access
    // -----------------------------------------------------------------------
    unsigned int addLevel();
    unsigned int addLevel(const XMLCh* const toSet, const unsigned int toSetLen,
                          const unsigned int readerNum);
    const StackElem* popTop();


    // -----------------------------------------------------------------------
    //  Stack top access
    // -----------------------------------------------------------------------
    const StackElem* topElement() const;
    void setElement(const XMLCh* const toSet, const unsigned int toSetLen,
                    const unsigned int readerNum);

    void setCurrentURI(unsigned int uri);
    unsigned int getCurrentURI();

    // -----------------------------------------------------------------------
    //  Prefix map methods
    // -----------------------------------------------------------------------
    void addPrefix
    (
        const   XMLCh* const    prefixToAdd
        , const unsigned int    uriId
    );
    unsigned int mapPrefixToURI
    (
        const   XMLCh* const    prefixToMap
        , const MapModes        mode
        ,       bool&           unknown
    )   const;


    // -----------------------------------------------------------------------
    //  Miscellaneous methods
    // -----------------------------------------------------------------------
    bool isEmpty() const;
    void reset
    (
        const   unsigned int    emptyId
        , const unsigned int    unknownId
        , const unsigned int    xmlId
        , const unsigned int    xmlNSId
    );


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    WFElemStack(const WFElemStack&);
    WFElemStack& operator=(const WFElemStack&);


    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    void expandMap();
    void expandStack();


    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fEmptyNamespaceId
    //      This is the special URI id for the "" namespace, which is magic
    //      because of the xmlns="" operation.
    //
    //  fGlobalPoolId
    //      This is a special URI id that is returned when the namespace
    //      prefix is "" and no one has explicitly mapped that prefix to an
    //      explicit URI (or when they explicitly clear any such mapping,
    //      which they can also do.) And also its prefix pool id, which is
    //      stored here for fast access.
    //
    //  fPrefixPool
    //      This is the prefix pool where prefixes are hashed and given unique
    //      ids. These ids are used to track prefixes in the element stack.
    //
    //  fStack
    //  fStackCapacity
    //  fStackTop
    //      This the stack array. Its an array of pointers to StackElem
    //      structures. The capacity is the current high water mark of the
    //      stack. The top is the current top of stack (i.e. the part of it
    //      being used.)
    //
    //  fUnknownNamespaceId
    //      This is the URI id for the special URI that is assigned to any
    //      prefix which has not been mapped. This lets us keep going after
    //      issuing the error.
    //
    //  fXMLNamespaceId
    //  fXMLPoolId
    //  fXMLNSNamespaceId
    //  fXMLNSPoolId
    //      These are the URI ids for the special URIs that are assigned to
    //      the 'xml' and 'xmlns' namespaces. And also its prefix pool id,
    //      which is stored here for fast access.
    // -----------------------------------------------------------------------
    unsigned int    fEmptyNamespaceId;
    unsigned int    fGlobalPoolId;
    unsigned int    fStackCapacity;
    unsigned int    fStackTop;
    unsigned int    fUnknownNamespaceId;
    unsigned int    fXMLNamespaceId;
    unsigned int    fXMLPoolId;
    unsigned int    fXMLNSNamespaceId;
    unsigned int    fXMLNSPoolId;
    unsigned int    fMapCapacity;
    PrefMapElem*    fMap;
    StackElem**     fStack;
    XMLStringPool   fPrefixPool;
    MemoryManager*  fMemoryManager;
};


// ---------------------------------------------------------------------------
//  ElemStack: Miscellaneous methods
// ---------------------------------------------------------------------------
inline bool ElemStack::isEmpty() const
{
    return (fStackTop == 0);
}

inline bool ElemStack::getValidationFlag()
{
    return fStack[fStackTop-1]->fValidationFlag;
}

inline void ElemStack::setValidationFlag(bool validationFlag)
{
    fStack[fStackTop-1]->fValidationFlag = validationFlag;
    return;
}

inline int ElemStack::getCurrentScope()
{
    return fStack[fStackTop-1]->fCurrentScope;
}

inline void ElemStack::setCurrentScope(int currentScope)
{
    fStack[fStackTop-1]->fCurrentScope = currentScope;
    return;
}

inline Grammar* ElemStack::getCurrentGrammar()
{
    return fStack[fStackTop-1]->fCurrentGrammar;
}

inline void ElemStack::setCurrentGrammar(Grammar* currentGrammar)
{
    fStack[fStackTop-1]->fCurrentGrammar = currentGrammar;
    return;
}

inline unsigned int ElemStack::getCurrentURI()
{
    return fStack[fStackTop-1]->fCurrentURI;
}

inline void ElemStack::setCurrentURI(unsigned int uri)
{
    fStack[fStackTop-1]->fCurrentURI = uri;
    return;
}

inline unsigned int ElemStack::getPrefixId(const XMLCh* const prefix) const
{
    return fPrefixPool.getId(prefix);
}

inline const XMLCh* ElemStack::getPrefixForId(unsigned int prefId) const
{
    return fPrefixPool.getValueForId(prefId);
}

// ---------------------------------------------------------------------------
//  WFElemStack: Miscellaneous methods
// ---------------------------------------------------------------------------
inline bool WFElemStack::isEmpty() const
{
    return (fStackTop == 0);
}

inline unsigned int WFElemStack::getCurrentURI()
{
    return fStack[fStackTop-1]->fCurrentURI;
}

inline void WFElemStack::setCurrentURI(unsigned int uri)
{
    fStack[fStackTop-1]->fCurrentURI = uri;
    return;
}


XERCES_CPP_NAMESPACE_END

#endif
