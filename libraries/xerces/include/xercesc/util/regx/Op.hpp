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
 * $Id: Op.hpp,v 1.11 2004/01/29 11:51:21 cargilld Exp $
 */

#if !defined(OP_HPP)
#define OP_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/util/RuntimeException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class Token;


class XMLUTIL_EXPORT Op : public XMemory
{
public:

    enum {
        O_DOT                = 0,
        O_CHAR               = 1,
        O_RANGE              = 3,
        O_NRANGE             = 4,
        O_ANCHOR             = 5,
        O_STRING             = 6,
        O_CLOSURE            = 7,
        O_NONGREEDYCLOSURE   = 8,
        O_QUESTION           = 9,
        O_NONGREEDYQUESTION  = 10,
        O_UNION              = 11,
        O_CAPTURE            = 15,
        O_BACKREFERENCE      = 16,
        O_LOOKAHEAD          = 20,
        O_NEGATIVELOOKAHEAD  = 21,
        O_LOOKBEHIND         = 22,
        O_NEGATIVELOOKBEHIND = 23,
        O_INDEPENDENT        = 24,
        O_MODIFIER           = 25,
        O_CONDITION          = 26
    };

    // -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
    virtual ~Op() { }

    // -----------------------------------------------------------------------
    // Getter functions
    // -----------------------------------------------------------------------
            short        getOpType() const;
            const Op*    getNextOp() const;
    virtual XMLInt32     getData() const;
    virtual XMLInt32     getData2() const;
    virtual int          getSize() const;
    virtual int          getRefNo() const;
    virtual const Op*    getConditionFlow() const;
    virtual const Op*    getYesFlow() const;
    virtual const Op*    getNoFlow() const;
    virtual const Op*    elementAt(int index) const;
    virtual const Op*    getChild() const;
    virtual const Token* getToken() const;
    virtual const XMLCh* getLiteral() const;

    // -----------------------------------------------------------------------
    // Setter functions
    // -----------------------------------------------------------------------
    void setOpType(const short type);
    void setNextOp(const Op* const next);

protected:
    // -----------------------------------------------------------------------
    //  Protected Constructors
    // -----------------------------------------------------------------------
    Op(const short type, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    friend class OpFactory;

    MemoryManager* const fMemoryManager;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    Op(const Op&);
    Op& operator=(const Op&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fOpType
    //      Indicates the type of operation
    //
    //  fNextOp
    //      Points to the next operation in the chain
    // -----------------------------------------------------------------------
    short fOpType;
    const Op*   fNextOp;
};


class XMLUTIL_EXPORT CharOp: public Op {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	CharOp(const short type, const XMLInt32 charData, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~CharOp() {}

	// -----------------------------------------------------------------------
	// Getter functions
	// -----------------------------------------------------------------------
	XMLInt32 getData() const;

private:
	// Private data members
	XMLInt32 fCharData;

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    CharOp(const CharOp&);
    CharOp& operator=(const CharOp&);
};

class XMLUTIL_EXPORT UnionOp : public Op {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	UnionOp(const short type, const int size,
            MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~UnionOp() { delete fBranches; }

	// -----------------------------------------------------------------------
	// Getter functions
	// -----------------------------------------------------------------------
	int getSize() const;
	const Op* elementAt(int index) const;

	// -----------------------------------------------------------------------
	// Setter functions
	// -----------------------------------------------------------------------
	void addElement(Op* const op);

private:
	// Private Data memebers
	RefVectorOf<Op>* fBranches;

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    UnionOp(const UnionOp&);
    UnionOp& operator=(const UnionOp&);
};


class XMLUTIL_EXPORT ChildOp: public Op {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	ChildOp(const short type, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~ChildOp() {}

	// -----------------------------------------------------------------------
	// Getter functions
	// -----------------------------------------------------------------------
	const Op* getChild() const;

	// -----------------------------------------------------------------------
	// Setter functions
	// -----------------------------------------------------------------------
	void setChild(const Op* const child);

private:
	// Private data members
	const Op* fChild;

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ChildOp(const ChildOp&);
    ChildOp& operator=(const ChildOp&);
};

class XMLUTIL_EXPORT ModifierOp: public ChildOp {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	ModifierOp(const short type, const XMLInt32 v1, const XMLInt32 v2, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~ModifierOp() {}

	// -----------------------------------------------------------------------
	// Getter functions
	// -----------------------------------------------------------------------
	XMLInt32 getData() const;
	XMLInt32 getData2() const;

private:
	// Private data members
	XMLInt32 fVal1;
	XMLInt32 fVal2;

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ModifierOp(const ModifierOp&);
    ModifierOp& operator=(const ModifierOp&);
};

class XMLUTIL_EXPORT RangeOp: public Op {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	RangeOp(const short type, const Token* const token, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~RangeOp() {}

	// -----------------------------------------------------------------------
	// Getter functions
	// -----------------------------------------------------------------------
	const Token* getToken() const;

private:
	// Private data members
	const Token* fToken;

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RangeOp(const RangeOp&);
    RangeOp& operator=(const RangeOp&);
};

class XMLUTIL_EXPORT StringOp: public Op {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	StringOp(const short type, const XMLCh* const literal, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~StringOp() { fMemoryManager->deallocate(fLiteral);}

	// -----------------------------------------------------------------------
	// Getter functions
	// -----------------------------------------------------------------------
	const XMLCh* getLiteral() const;

private:
	// Private data members
	XMLCh* fLiteral;

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    StringOp(const StringOp&);
    StringOp& operator=(const StringOp&);
};

class XMLUTIL_EXPORT ConditionOp: public Op {
public:
	// -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	ConditionOp(const short type, const int refNo,
				const Op* const condFlow, const Op* const yesFlow,
				const Op* const noFlow, MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~ConditionOp() {}

	// -----------------------------------------------------------------------
	// Getter functions
	// -----------------------------------------------------------------------
	int			getRefNo() const;
	const Op*	getConditionFlow() const;
	const Op*	getYesFlow() const;
	const Op*	getNoFlow() const;
	
private:
	// Private data members
	int fRefNo;
	const Op* fConditionOp;
	const Op* fYesOp;
	const Op* fNoOp;

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ConditionOp(const ConditionOp&);
    ConditionOp& operator=(const ConditionOp&);
};

// ---------------------------------------------------------------------------
//  Op: getter methods
// ---------------------------------------------------------------------------
inline short Op::getOpType() const {

	return fOpType;
}

inline const Op* Op::getNextOp() const {

	return fNextOp;
}

// ---------------------------------------------------------------------------
//  Op: setter methods
// ---------------------------------------------------------------------------
inline void Op::setOpType(const short type) {

	fOpType = type;
}

inline void Op::setNextOp(const Op* const nextOp) {
	
	fNextOp = nextOp;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file Op.hpp
  */
