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
 * $Id: XercesXPath.hpp,v 1.8 2003/11/06 15:30:08 neilg Exp $
 */

#if !defined(XERCESXPATH_HPP)
#define XERCESXPATH_HPP


// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/QName.hpp>
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/util/ValueVectorOf.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declarations
// ---------------------------------------------------------------------------
class XMLStringPool;
class NamespaceScope;


class VALIDATORS_EXPORT XercesNodeTest : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constants
    // -----------------------------------------------------------------------
    enum {
        QNAME = 1,
        WILDCARD = 2,
        NODE = 3,
        NAMESPACE= 4,
        UNKNOWN
    };

    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    XercesNodeTest(const short type,
                   MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    XercesNodeTest(const QName* const qName);
    XercesNodeTest(const XMLCh* const prefix, const unsigned int uriId,
                   MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    XercesNodeTest(const XercesNodeTest& other);
    ~XercesNodeTest() { delete fName; }

    // -----------------------------------------------------------------------
    //  Operators
    // -----------------------------------------------------------------------
    XercesNodeTest& operator= (const XercesNodeTest& other);
    bool operator== (const XercesNodeTest& other) const;
    bool operator!= (const XercesNodeTest& other) const;

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    short getType() const { return fType; }
    QName* getName() const { return fName; }

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XercesNodeTest)

    XercesNodeTest(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    short  fType;
    QName* fName;
};


/**
  * A location path step comprised of an axis and node test.
  */
class VALIDATORS_EXPORT XercesStep : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constants
    // -----------------------------------------------------------------------
    enum { // Axis type
        CHILD = 1,
        ATTRIBUTE = 2,
        SELF = 3,
        DESCENDANT = 4,
        UNKNOWN
    };

    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    XercesStep(const unsigned short axisType, XercesNodeTest* const nodeTest);
    XercesStep(const XercesStep& other);
    ~XercesStep() { delete fNodeTest; }

    // -----------------------------------------------------------------------
    //  Operators
    // -----------------------------------------------------------------------
    XercesStep& operator= (const XercesStep& other);
    bool operator== (const XercesStep& other) const;
    bool operator!= (const XercesStep& other) const;

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    unsigned short getAxisType() const { return fAxisType; }
    XercesNodeTest* getNodeTest() const { return fNodeTest; }

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XercesStep)

    XercesStep(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    unsigned short  fAxisType;
    XercesNodeTest* fNodeTest;
};


/**
  * A location path representation for an XPath expression.
  */
class VALIDATORS_EXPORT XercesLocationPath : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    XercesLocationPath(RefVectorOf<XercesStep>* const steps);
    ~XercesLocationPath() { delete fSteps; }

    // -----------------------------------------------------------------------
    //  Operators
    // -----------------------------------------------------------------------
    bool operator== (const XercesLocationPath& other) const;
    bool operator!= (const XercesLocationPath& other) const;

    // -----------------------------------------------------------------------
    //  Access methods
    // -----------------------------------------------------------------------
    unsigned int getStepSize() const;
    void addStep(XercesStep* const aStep);
    XercesStep* getStep(const unsigned int index) const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XercesLocationPath)

    XercesLocationPath(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    XercesLocationPath(const XercesLocationPath& other);
    XercesLocationPath& operator= (const XercesLocationPath& other);

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    RefVectorOf<XercesStep>* fSteps;
};


class VALIDATORS_EXPORT XercesXPath : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constants
    // -----------------------------------------------------------------------
    /**
      * [28] ExprToken ::= '(' | ')' | '[' | ']' | '.' | '..' | '@' | ',' | '::'
      *                  | NameTest | NodeType | Operator | FunctionName
      *                  | AxisName | Literal | Number | VariableReference
      */
    enum {
        EXPRTOKEN_OPEN_PAREN                  =  0,
        EXPRTOKEN_CLOSE_PAREN                 =  1,
        EXPRTOKEN_OPEN_BRACKET                =  2,
        EXPRTOKEN_CLOSE_BRACKET               =  3,
        EXPRTOKEN_PERIOD                      =  4,
        EXPRTOKEN_DOUBLE_PERIOD               =  5,
        EXPRTOKEN_ATSIGN                      =  6,
        EXPRTOKEN_COMMA                       =  7,
        EXPRTOKEN_DOUBLE_COLON                =  8,
        EXPRTOKEN_NAMETEST_ANY                =  9,
        EXPRTOKEN_NAMETEST_NAMESPACE          = 10,
        EXPRTOKEN_NAMETEST_QNAME              = 11,
        EXPRTOKEN_NODETYPE_COMMENT            = 12,
        EXPRTOKEN_NODETYPE_TEXT               = 13,
        EXPRTOKEN_NODETYPE_PI                 = 14,
        EXPRTOKEN_NODETYPE_NODE               = 15,
        EXPRTOKEN_OPERATOR_AND                = 16,
        EXPRTOKEN_OPERATOR_OR                 = 17,
        EXPRTOKEN_OPERATOR_MOD                = 18,
        EXPRTOKEN_OPERATOR_DIV                = 19,
        EXPRTOKEN_OPERATOR_MULT               = 20,
        EXPRTOKEN_OPERATOR_SLASH              = 21,
        EXPRTOKEN_OPERATOR_DOUBLE_SLASH       = 22,
        EXPRTOKEN_OPERATOR_UNION              = 23,
        EXPRTOKEN_OPERATOR_PLUS               = 24,
        EXPRTOKEN_OPERATOR_MINUS              = 25,
        EXPRTOKEN_OPERATOR_EQUAL              = 26,
        EXPRTOKEN_OPERATOR_NOT_EQUAL          = 27,
        EXPRTOKEN_OPERATOR_LESS               = 28,
        EXPRTOKEN_OPERATOR_LESS_EQUAL         = 29,
        EXPRTOKEN_OPERATOR_GREATER            = 30,
        EXPRTOKEN_OPERATOR_GREATER_EQUAL      = 31,
        EXPRTOKEN_FUNCTION_NAME               = 32,
        EXPRTOKEN_AXISNAME_ANCESTOR           = 33,
        EXPRTOKEN_AXISNAME_ANCESTOR_OR_SELF   = 34,
        EXPRTOKEN_AXISNAME_ATTRIBUTE          = 35,
        EXPRTOKEN_AXISNAME_CHILD              = 36,
        EXPRTOKEN_AXISNAME_DESCENDANT         = 37,
        EXPRTOKEN_AXISNAME_DESCENDANT_OR_SELF = 38,
        EXPRTOKEN_AXISNAME_FOLLOWING          = 39,
        EXPRTOKEN_AXISNAME_FOLLOWING_SIBLING  = 40,
        EXPRTOKEN_AXISNAME_NAMESPACE          = 41,
        EXPRTOKEN_AXISNAME_PARENT             = 42,
        EXPRTOKEN_AXISNAME_PRECEDING          = 43,
        EXPRTOKEN_AXISNAME_PRECEDING_SIBLING  = 44,
        EXPRTOKEN_AXISNAME_SELF               = 45,
        EXPRTOKEN_LITERAL                     = 46,
        EXPRTOKEN_NUMBER                      = 47,
        EXPRTOKEN_VARIABLE_REFERENCE          = 48
    };

    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    XercesXPath(const XMLCh* const xpathExpr,
                XMLStringPool* const stringPool,
                NamespaceScope* const scopeContext,
                const unsigned int emptyNamespaceId,
                const bool isSelector = false,
                MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	~XercesXPath();

    // -----------------------------------------------------------------------
    //  Operators
    // -----------------------------------------------------------------------
    bool operator== (const XercesXPath& other) const;
    bool operator!= (const XercesXPath& other) const;

    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    RefVectorOf<XercesLocationPath>* getLocationPaths() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XercesXPath)

    XercesXPath(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    XMLCh* getExpression();

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    XercesXPath(const XercesXPath& other);
    XercesXPath& operator= (const XercesXPath& other);

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void cleanUp();
    void checkForSelectedAttributes();
    void parseExpression(XMLStringPool* const stringPool,
                         NamespaceScope* const scopeContext);

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    unsigned int                     fEmptyNamespaceId;
    XMLCh*                           fExpression;
    RefVectorOf<XercesLocationPath>* fLocationPaths;
    MemoryManager*                   fMemoryManager;
};


class VALIDATORS_EXPORT XPathScanner : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constants
    // -----------------------------------------------------------------------
    enum {
        CHARTYPE_INVALID            =  0,   // invalid XML character
        CHARTYPE_OTHER              =  1,   // not special - one of "#%&;?\^`{}~" or DEL
        CHARTYPE_WHITESPACE         =  2,   // one of "\t\n\r " (0x09, 0x0A, 0x0D, 0x20)
        CHARTYPE_EXCLAMATION        =  3,   // '!' (0x21)
        CHARTYPE_QUOTE              =  4,   // '\"' or '\'' (0x22 and 0x27)
        CHARTYPE_DOLLAR             =  5,   // '$' (0x24)
        CHARTYPE_OPEN_PAREN         =  6,   // '(' (0x28)
        CHARTYPE_CLOSE_PAREN        =  7,   // ')' (0x29)
        CHARTYPE_STAR               =  8,   // '*' (0x2A)
        CHARTYPE_PLUS               =  9,   // '+' (0x2B)
        CHARTYPE_COMMA              = 10,   // ',' (0x2C)
        CHARTYPE_MINUS              = 11,   // '-' (0x2D)
        CHARTYPE_PERIOD             = 12,   // '.' (0x2E)
        CHARTYPE_SLASH              = 13,   // '/' (0x2F)
        CHARTYPE_DIGIT              = 14,   // '0'-'9' (0x30 to 0x39)
        CHARTYPE_COLON              = 15,   // ':' (0x3A)
        CHARTYPE_LESS               = 16,   // '<' (0x3C)
        CHARTYPE_EQUAL              = 17,   // '=' (0x3D)
        CHARTYPE_GREATER            = 18,   // '>' (0x3E)
        CHARTYPE_ATSIGN             = 19,   // '@' (0x40)
        CHARTYPE_LETTER             = 20,   // 'A'-'Z' or 'a'-'z' (0x41 to 0x5A and 0x61 to 0x7A)
        CHARTYPE_OPEN_BRACKET       = 21,   // '[' (0x5B)
        CHARTYPE_CLOSE_BRACKET      = 22,   // ']' (0x5D)
        CHARTYPE_UNDERSCORE         = 23,   // '_' (0x5F)
        CHARTYPE_UNION              = 24,   // '|' (0x7C)
        CHARTYPE_NONASCII           = 25   // Non-ASCII Unicode codepoint (>= 0x80)
    };

    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    XPathScanner(XMLStringPool* const stringPool);
    virtual ~XPathScanner() {}

    // -----------------------------------------------------------------------
    //  Scan methods
    // -----------------------------------------------------------------------
    bool scanExpression(const XMLCh* const data, int currentOffset,
                        const int endOffset, ValueVectorOf<int>* const tokens);

protected:
    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    /**
      * This method adds the specified token to the token list. By default,
      * this method allows all tokens. However, subclasses can can override
      * this method in order to disallow certain tokens from being used in the
      * scanned XPath expression. This is a convenient way of allowing only
      * a subset of XPath.
      */
    virtual void addToken(ValueVectorOf<int>* const tokens, const int aToken);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    XPathScanner(const XPathScanner& other);
    XPathScanner& operator= (const XPathScanner& other);

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void init();

    // -----------------------------------------------------------------------
    //  Scan methods
    // -----------------------------------------------------------------------
    int scanNCName(const XMLCh* const data, const int endOffset,
                   int currentOffset);
    int scanNumber(const XMLCh* const data, const int endOffset,
                   int currentOffset, ValueVectorOf<int>* const tokens);

    // -----------------------------------------------------------------------
    //  Data members
    // -----------------------------------------------------------------------
    int fAndSymbol;
    int fOrSymbol;
    int fModSymbol;
    int fDivSymbol;
    int fCommentSymbol;
    int fTextSymbol;
    int fPISymbol;
    int fNodeSymbol;
    int fAncestorSymbol;
    int fAncestorOrSelfSymbol;
    int fAttributeSymbol;
    int fChildSymbol;
    int fDescendantSymbol;
    int fDescendantOrSelfSymbol;
    int fFollowingSymbol;
    int fFollowingSiblingSymbol;
    int fNamespaceSymbol;
    int fParentSymbol;
    int fPrecedingSymbol;
    int fPrecedingSiblingSymbol;
    int fSelfSymbol;
    XMLStringPool* fStringPool;

    static const XMLByte fASCIICharMap[128];
};


class VALIDATORS_EXPORT XPathScannerForSchema: public XPathScanner
{
public:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    XPathScannerForSchema(XMLStringPool* const stringPool);
    ~XPathScannerForSchema() {}

protected:
    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void addToken(ValueVectorOf<int>* const tokens, const int aToken);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    XPathScannerForSchema(const XPathScannerForSchema& other);
    XPathScannerForSchema& operator= (const XPathScannerForSchema& other);
};

// ---------------------------------------------------------------------------
//  XercesLocationPath: Access methods
// ---------------------------------------------------------------------------
inline unsigned int XercesLocationPath::getStepSize() const {

    if (fSteps)
        return fSteps->size();

    return 0;
}

inline void XercesLocationPath::addStep(XercesStep* const aStep) {

    fSteps->addElement(aStep);
}

inline XercesStep* XercesLocationPath::getStep(const unsigned int index) const {

    if (fSteps)
        return fSteps->elementAt(index);

    return 0;
}

// ---------------------------------------------------------------------------
//  XercesScanner: Helper methods
// ---------------------------------------------------------------------------
inline void XPathScanner::addToken(ValueVectorOf<int>* const tokens,
                                   const int aToken) {
    tokens->addElement(aToken);
}


// ---------------------------------------------------------------------------
//  XercesXPath: Getter methods
// ---------------------------------------------------------------------------
inline RefVectorOf<XercesLocationPath>* XercesXPath::getLocationPaths() const {

    return fLocationPaths;
}

inline XMLCh* XercesXPath::getExpression() {
    return fExpression;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file XercesPath.hpp
  */

