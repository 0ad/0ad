/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2003 The Apache Software Foundation.  All rights
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
 * $Id: OpFactory.hpp,v 1.6 2003/05/22 02:10:52 knoaman Exp $
 */

#if !defined(OPFACTORY_HPP)
#define OPFACTORY_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/RefVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class Op;
class CharOp;
class UnionOp;
class ChildOp;
class RangeOp;
class StringOp;
class ModifierOp;
class ConditionOp;
class Token;

/*
 * A Factory class used by 'RegularExpression' to create different types of
 * operations (Op) objects. The class will keep track of all objects created
 * for cleanup purposes. Each 'RegularExpression' object will have its own
 * instance of OpFactory and when a 'RegularExpression' object is deleted
 * all associated Op objects will be deleted.
 */

class XMLUTIL_EXPORT OpFactory : public XMemory
{
public:
	// -----------------------------------------------------------------------
    //  Constructors and destructors
    // -----------------------------------------------------------------------
	OpFactory(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    ~OpFactory();

    // -----------------------------------------------------------------------
    //  Factory methods
    // -----------------------------------------------------------------------
    Op* createDotOp();
	CharOp* createCharOp(XMLInt32 data);
	CharOp* createAnchorOp(XMLInt32 data);
	CharOp* createCaptureOp(int number, const Op* const next);
	UnionOp* createUnionOp(int size);
	ChildOp* createClosureOp(int id);
	ChildOp* createNonGreedyClosureOp();
	ChildOp* createQuestionOp(bool nonGreedy);
	RangeOp* createRangeOp(const Token* const token);
	ChildOp* createLookOp(const short type, const Op* const next,
                          const Op* const branch);
	CharOp* createBackReferenceOp(int refNo);
	StringOp* createStringOp(const XMLCh* const literal);
	ChildOp* createIndependentOp(const Op* const next,
								 const Op* const branch);
	ModifierOp* createModifierOp(const Op* const next, const Op* const branch,
								 const int add, const int mask);
	ConditionOp* createConditionOp(const Op* const next, const int ref,
                                   const Op* const conditionFlow,
                                   const Op* const yesFlow,
                                   const Op* const noFlow);

    // -----------------------------------------------------------------------
    //  Reset methods
    // -----------------------------------------------------------------------
	/*
	 *	Remove all created Op objects from Vector
	 */
	void reset();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    OpFactory(const OpFactory&);
    OpFactory& operator=(const OpFactory&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fOpVector
    //      Contains Op objects. Used for memory cleanup.
    // -----------------------------------------------------------------------
    RefVectorOf<Op>* fOpVector;
    MemoryManager*   fMemoryManager;
};

// ---------------------------------------------------------------------------
//  OpFactory - Factory methods
// ---------------------------------------------------------------------------
inline void OpFactory::reset() {

	fOpVector->removeAllElements();
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  *	End file OpFactory
  */
