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
 * $Log: DFAContentModel.hpp,v $
 * Revision 1.6  2003/12/17 00:18:38  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.5  2003/05/16 21:43:20  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.4  2003/05/15 18:48:27  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2003/03/07 18:16:57  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.2  2002/11/04 14:54:58  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:38  peiyongz
 * sane_include
 *
 * Revision 1.13  2001/11/21 14:30:13  knoaman
 * Fix for UPA checking.
 *
 * Revision 1.12  2001/08/24 12:48:48  tng
 * Schema: AllContentModel
 *
 * Revision 1.11  2001/08/21 16:06:11  tng
 * Schema: Unique Particle Attribution Constraint Checking.
 *
 * Revision 1.10  2001/08/13 15:06:39  knoaman
 * update <any> validation.
 *
 * Revision 1.9  2001/06/13 20:50:55  peiyongz
 * fIsMixed: to handle mixed Content Model
 *
 * Revision 1.8  2001/05/11 13:27:18  tng
 * Copyright update.
 *
 * Revision 1.7  2001/05/03 21:02:30  tng
 * Schema: Add SubstitutionGroupComparator and update exception messages.  By Pei Yong Zhang.
 *
 * Revision 1.6  2001/04/19 18:17:30  tng
 * Schema: SchemaValidator update, and use QName in Content Model
 *
 * Revision 1.5  2001/03/21 21:56:27  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 * Revision 1.4  2001/03/21 19:29:55  tng
 * Schema: Content Model Updates, by Pei Yong Zhang.
 *
 * Revision 1.3  2001/02/27 18:32:32  tng
 * Schema: Use XMLElementDecl instead of DTDElementDecl in Content Model.
 *
 * Revision 1.2  2001/02/27 14:48:52  tng
 * Schema: Add CMAny and ContentLeafNameTypeVector, by Pei Yong Zhang
 *
 * Revision 1.1  2001/02/16 14:17:29  tng
 * Schema: Move the common Content Model files that are shared by DTD
 * and schema from 'DTD' folder to 'common' folder.  By Pei Yong Zhang.
 *
 * Revision 1.4  2000/03/02 19:55:38  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.3  2000/02/24 20:16:48  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/09 21:42:37  abagchi
 * Copyright swat
 *
 * Revision 1.1.1.1  1999/11/09 01:03:19  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:38  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#if !defined(DFACONTENTMODEL_HPP)
#define DFACONTENTMODEL_HPP

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/ArrayIndexOutOfBoundsException.hpp>
#include <xercesc/framework/XMLContentModel.hpp>
#include <xercesc/validators/common/ContentLeafNameTypeVector.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class ContentSpecNode;
class CMLeaf;
class CMNode;
class CMStateSet;

//
//  DFAContentModel is the heavy weight derivative of ContentModel that does
//  all of the non-trivial element content validation. This guy does the full
//  bore regular expression to DFA conversion to create a DFA that it then
//  uses in its validation algorithm.
//
//  NOTE:   Upstream work insures that this guy will never see a content model
//          with PCDATA in it. Any model with PCDATA is 'mixed' and is handled
//          via the MixedContentModel class, since mixed models are very
//          constrained in form and easily handled via a special case. This
//          also makes our life much easier here.
//
class DFAContentModel : public XMLContentModel
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    DFAContentModel
    ( 
          const bool             dtd
        , ContentSpecNode* const elemContentSpec
        , MemoryManager* const   manager = XMLPlatformUtils::fgMemoryManager
    );
    DFAContentModel
    (
          const bool             dtd
        , ContentSpecNode* const elemContentSpec
        , const bool             isMixed
        , MemoryManager* const   manager
    );

    virtual ~DFAContentModel();


    // -----------------------------------------------------------------------
    //  Implementation of the virtual content model interface
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

    virtual void checkUniqueParticleAttribution
    (
        SchemaGrammar*    const pGrammar
      , GrammarResolver*  const pGrammarResolver
      , XMLStringPool*    const pStringPool
      , XMLValidator*     const pValidator
      , unsigned int*     const pContentSpecOrgURI
    ) ;

    virtual ContentLeafNameTypeVector* getContentLeafNameTypeVector() const ;

    virtual unsigned int getNextState(const unsigned int currentState,
                                      const unsigned int elementIndex) const;

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DFAContentModel();
    DFAContentModel(const DFAContentModel&);
    DFAContentModel& operator=(const DFAContentModel&);


    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    void buildDFA(ContentSpecNode* const curNode);
    CMNode* buildSyntaxTree(ContentSpecNode* const curNode);
    void calcFollowList(CMNode* const curNode);
    unsigned int* makeDefStateList() const;
    int postTreeBuildInit
    (
                CMNode* const   nodeCur
        , const unsigned int    curIndex
    );


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fElemMap
    //  fElemMapSize
    //      This is the map of unique input symbol elements to indices into
    //      each state's per-input symbol transition table entry. This is part
    //      of the built DFA information that must be kept around to do the
    //      actual validation.
    //
    //  fElemMapType
    //      This is a map of whether the element map contains information
    //      related to ANY models.
    //
    //  fEmptyOk
    //      This is an optimization. While building the transition table we
    //      can see whether this content model would approve of an empty
    //      content (which could happen if everything was optional.) So we
    //      set this flag and short circuit that check, which would otherwise
    //      be ugly and time consuming if we tried to determine it at each
    //      validation call.
    //
    //  fEOCPos
    //      The NFA position of the special EOC (end of content) node. This
    //      is saved away since its used during the DFA build.
    //
    //  fFinalStateFlags
    //      This is an array of booleans, one per state (there are
    //      fTransTableSize states in the DFA) that indicates whether that
    //      state is a final state.
    //
    //  fFollowList
    //      The list of follow positions for each NFA position (i.e. for each
    //      non-epsilon leaf node.) This is only used during the building of
    //      the DFA, and is let go afterwards.
    //
    //  fHeadNode
    //      This is the head node of our intermediate representation. It is
    //      only non-null during the building of the DFA (just so that it
    //      does not have to be passed all around.) Once the DFA is built,
    //      this is no longer required so its deleted.
    //
    //  fLeafCount
    //      The count of leaf nodes. This is an important number that set some
    //      limits on the sizes of data structures in the DFA process.
    //
    //  fLeafList
    //      An array of non-epsilon leaf nodes, which is used during the DFA
    //      build operation, then dropped. These are just references to nodes
    //      pointed to by fHeadNode, so we don't have to clean them up, just
    //      the actually leaf list array itself needs cleanup.
    //
    //  fLeafListType
    //      Array mapping ANY types to the leaf list.
    //
    //  fTransTable
    //  fTransTableSize
    //      This is the transition table that is the main by product of all
    //      of the effort here. It is an array of arrays of ints. The first
    //      dimension is the number of states we end up with in the DFA. The
    //      second dimensions is the number of unique elements in the content
    //      model (fElemMapSize). Each entry in the second dimension indicates
    //      the new state given that input for the first dimension's start
    //      state.
    //
    //      The fElemMap array handles mapping from element indexes to
    //      positions in the second dimension of the transition table.
    //
    //      fTransTableSize is the number of valid entries in the transition
    //      table, and in the other related tables such as fFinalStateFlags.
    //
    //  fDTD
    //      Boolean to allow DTDs to validate even with namespace support.
    //
    //  fIsMixed
    //      DFA ContentModel with mixed PCDATA.
    // -----------------------------------------------------------------------
    QName**                 fElemMap;
    ContentSpecNode::NodeTypes  *fElemMapType;
    unsigned int            fElemMapSize;
    bool                    fEmptyOk;
    unsigned int            fEOCPos;
    bool*                   fFinalStateFlags;
    CMStateSet**            fFollowList;
    CMNode*                 fHeadNode;
    unsigned int            fLeafCount;
    CMLeaf**                fLeafList;
    ContentSpecNode::NodeTypes  *fLeafListType;
    unsigned int**          fTransTable;
    unsigned int            fTransTableSize;
    bool                    fDTD;
    bool                    fIsMixed;
    ContentLeafNameTypeVector *fLeafNameTypeVector;
    MemoryManager*             fMemoryManager;
};


inline unsigned int
DFAContentModel::getNextState(const unsigned int currentState,
                              const unsigned int elementIndex) const {

    if (currentState == XMLContentModel::gInvalidTrans) {
        return XMLContentModel::gInvalidTrans;
    }

    if (currentState >= fTransTableSize || elementIndex >= fElemMapSize) {
        ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Array_BadIndex, fMemoryManager);
    }

    return fTransTable[currentState][elementIndex];
}

XERCES_CPP_NAMESPACE_END

#endif

