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
 * $Log: SimpleContentModel.hpp,v $
 * Revision 1.9  2004/01/29 11:51:21  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.8  2003/12/17 00:18:38  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.7  2003/06/03 18:12:29  knoaman
 * Add default value for memory manager argument.
 *
 * Revision 1.6  2003/05/18 14:02:06  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.5  2003/05/15 18:48:27  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.4  2003/03/07 18:16:57  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.3  2002/11/04 14:54:58  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/10/30 21:52:00  tng
 * [Bug 13641] compiler-generated copy-constructor for QName doesn't do the right thing.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:39  peiyongz
 * sane_include
 *
 * Revision 1.12  2001/11/21 14:30:13  knoaman
 * Fix for UPA checking.
 *
 * Revision 1.11  2001/10/03 15:08:45  tng
 * typo fix: remove the extra space which may confuse some compilers while constructing the qname.
 *
 * Revision 1.10  2001/08/21 16:06:11  tng
 * Schema: Unique Particle Attribution Constraint Checking.
 *
 * Revision 1.9  2001/08/13 15:06:39  knoaman
 * update <any> validation.
 *
 * Revision 1.8  2001/05/11 13:27:20  tng
 * Copyright update.
 *
 * Revision 1.7  2001/05/03 21:02:33  tng
 * Schema: Add SubstitutionGroupComparator and update exception messages.  By Pei Yong Zhang.
 *
 * Revision 1.6  2001/04/19 18:17:34  tng
 * Schema: SchemaValidator update, and use QName in Content Model
 *
 * Revision 1.5  2001/03/21 21:56:29  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 * Revision 1.4  2001/03/21 19:30:02  tng
 * Schema: Content Model Updates, by Pei Yong Zhang.
 *
 * Revision 1.3  2001/02/27 14:48:57  tng
 * Schema: Add CMAny and ContentLeafNameTypeVector, by Pei Yong Zhang
 *
 * Revision 1.2  2001/02/16 14:58:57  tng
 * Schema: Update Makefile, configure files, project files, and include path in
 * certain cpp files because of the move of the common Content Model files.  By Pei Yong Zhang.
 *
 * Revision 1.1  2001/02/16 14:17:29  tng
 * Schema: Move the common Content Model files that are shared by DTD
 * and schema from 'DTD' folder to 'common' folder.  By Pei Yong Zhang.
 *
 * Revision 1.3  2000/02/24 20:16:49  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/09 21:42:39  abagchi
 * Copyright swat
 *
 * Revision 1.1.1.1  1999/11/09 01:03:48  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:44  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(SIMPLECONTENTMODEL_HPP)
#define SIMPLECONTENTMODEL_HPP

#include <xercesc/framework/XMLContentModel.hpp>
#include <xercesc/validators/common/ContentSpecNode.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  SimpleContentModel is a derivative of the abstract content model base
//  class that handles a small set of simple content models that are just
//  way overkill to give the DFA treatment.
//
//  DESCRIPTION:
//
//  This guy handles the following scenarios:
//
//      a
//      a?
//      a*
//      a+
//      a,b
//      a|b
//
//  These all involve a unary operation with one element type, or a binary
//  operation with two elements. These are very simple and can be checked
//  in a simple way without a DFA and without the overhead of setting up a
//  DFA for such a simple check.
//
//  NOTE:   Pass the XMLElementDecl::fgPCDataElemId value to represent a
//          PCData node. Pass XMLElementDecl::fgInvalidElemId for unused element
//
class SimpleContentModel : public XMLContentModel
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    SimpleContentModel
    (
        const bool                        dtd
      , QName* const                      firstChild
      , QName* const                      secondChild
      , const ContentSpecNode::NodeTypes  cmOp
      , MemoryManager* const              manager = XMLPlatformUtils::fgMemoryManager
    );

    ~SimpleContentModel();


    // -----------------------------------------------------------------------
    //  Implementation of the ContentModel virtual interface
    // -----------------------------------------------------------------------
	virtual int validateContent
    (
        QName** const         children
      , const unsigned int    childCount
      , const unsigned int    emptyNamespaceId
    ) const;

	virtual int validateContentSpecial
    (
        QName** const           children
      , const unsigned int      childCount
      , const unsigned int      emptyNamespaceId
      , GrammarResolver*  const pGrammarResolver
      , XMLStringPool*    const pStringPool
    ) const;

    virtual ContentLeafNameTypeVector *getContentLeafNameTypeVector() const;

    virtual unsigned int getNextState(const unsigned int currentState,
                                      const unsigned int elementIndex) const;

    virtual void checkUniqueParticleAttribution
    (
        SchemaGrammar*    const pGrammar
      , GrammarResolver*  const pGrammarResolver
      , XMLStringPool*    const pStringPool
      , XMLValidator*     const pValidator
      , unsigned int*     const pContentSpecOrgURI
    ) ;

 private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    SimpleContentModel();
    SimpleContentModel(const SimpleContentModel&);
    SimpleContentModel& operator=(const SimpleContentModel&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fFirstChild
    //  fSecondChild
    //      The first (and optional second) child node. The
    //      operation code tells us whether the second child is used or not.
    //
    //  fOp
    //      The operation that this object represents. Since this class only
    //      does simple contents, there is only ever a single operation
    //      involved (i.e. the children of the operation are always one or
    //      two leafs.)
    //
    //  fDTD
    //      Boolean to allow DTDs to validate even with namespace support. */
    //
    // -----------------------------------------------------------------------
    QName*                     fFirstChild;
    QName*                     fSecondChild;
    ContentSpecNode::NodeTypes fOp;
    bool                       fDTD;
    MemoryManager* const       fMemoryManager;
};


// ---------------------------------------------------------------------------
//  SimpleContentModel: Constructors and Destructor
// ---------------------------------------------------------------------------
inline SimpleContentModel::SimpleContentModel
(
      const bool                       dtd
    , QName* const                     firstChild
    , QName* const                     secondChild
    , const ContentSpecNode::NodeTypes cmOp
     , MemoryManager* const            manager
)
    : fFirstChild(0)
    , fSecondChild(0)
    , fOp(cmOp)
	, fDTD(dtd)
    , fMemoryManager(manager)
{
    if (firstChild)
        fFirstChild = new (manager) QName(*firstChild);
    else
        fFirstChild = new (manager) QName(XMLUni::fgZeroLenString, XMLUni::fgZeroLenString, XMLElementDecl::fgInvalidElemId, manager);

    if (secondChild)
        fSecondChild = new (manager) QName(*secondChild);
    else
        fSecondChild = new (manager) QName(XMLUni::fgZeroLenString, XMLUni::fgZeroLenString, XMLElementDecl::fgInvalidElemId, manager);
}

inline SimpleContentModel::~SimpleContentModel()
{
    delete fFirstChild;
    delete fSecondChild;
}


// ---------------------------------------------------------------------------
//  SimpleContentModel: Virtual methods
// ---------------------------------------------------------------------------
inline unsigned int
SimpleContentModel::getNextState(const unsigned int,
                                 const unsigned int) const {

    return XMLContentModel::gInvalidTrans;
}

XERCES_CPP_NAMESPACE_END

#endif
