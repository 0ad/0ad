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
 * $Id: IdentityConstraint.hpp,v 1.7 2003/11/14 22:35:09 neilg Exp $
 */

#if !defined(IDENTITYCONSTRAINT_HPP)
#define IDENTITYCONSTRAINT_HPP


/**
  * The class act as a base class for schema identity constraints.
  */

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/validators/schema/identity/IC_Field.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declarations
// ---------------------------------------------------------------------------
class IC_Selector;

class VALIDATORS_EXPORT IdentityConstraint : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constants
    // -----------------------------------------------------------------------
    enum ICType {
        UNIQUE = 0,
        KEY = 1,
        KEYREF = 2,
        UNKNOWN
    };

    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
	virtual ~IdentityConstraint();

    // -----------------------------------------------------------------------
    //  operators
    // -----------------------------------------------------------------------
    bool operator== (const IdentityConstraint& other) const;
    bool operator!= (const IdentityConstraint& other) const;

	// -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    virtual short getType() const = 0;
    int           getFieldCount() const;
    XMLCh*        getIdentityConstraintName() const;
    XMLCh*        getElementName() const;
    IC_Selector*  getSelector() const;
    int           getNamespaceURI() const;

	// -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setSelector(IC_Selector* const selector);
    void setNamespaceURI(int uri);

	// -----------------------------------------------------------------------
    //  Access methods
    // -----------------------------------------------------------------------
    void addField(IC_Field* const field);
    const IC_Field* getFieldAt(const unsigned int index) const;
    IC_Field* getFieldAt(const unsigned int index);

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(IdentityConstraint)

	static void                storeIC(XSerializeEngine&         serEng
                                     , IdentityConstraint* const ic);

	static IdentityConstraint* loadIC(XSerializeEngine& serEng);

protected:
    // -----------------------------------------------------------------------
    //  Constructors/Destructor
    // -----------------------------------------------------------------------
    IdentityConstraint(const XMLCh* const identityConstraintName,
                       const XMLCh* const elementName,
					   MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented contstructors and operators
    // -----------------------------------------------------------------------
    IdentityConstraint(const IdentityConstraint& other);
    IdentityConstraint& operator= (const IdentityConstraint& other);

    // -----------------------------------------------------------------------
    //  CleanUp methods
    // -----------------------------------------------------------------------
    void cleanUp();

    // -----------------------------------------------------------------------
    //  Data members
    //
    //  fIdentityConstraintName
    //      The identity constraint name
    //
    //  fElemName
    //      The element name
    //
    //  fSelector
    //      The selector information
    //
    //  fFields
    //      The field(s) information
    // -----------------------------------------------------------------------
    XMLCh*                 fIdentityConstraintName;
    XMLCh*                 fElemName;
    IC_Selector*           fSelector;
    RefVectorOf<IC_Field>* fFields;
    MemoryManager*         fMemoryManager;
    int                    fNamespaceURI;
};


// ---------------------------------------------------------------------------
//  IdentityConstraint: Getter methods
// ---------------------------------------------------------------------------
inline int IdentityConstraint::getFieldCount() const {

    if (fFields) {
        return fFields->size();
    }

    return 0;
}

inline XMLCh* IdentityConstraint::getIdentityConstraintName() const {

    return fIdentityConstraintName;
}

inline XMLCh* IdentityConstraint::getElementName() const {

    return fElemName;
}

inline IC_Selector* IdentityConstraint::getSelector() const {

    return fSelector;
}

inline int IdentityConstraint::getNamespaceURI() const
{
    return fNamespaceURI;
}

// ---------------------------------------------------------------------------
//  IdentityConstraint: Setter methods
// ---------------------------------------------------------------------------
inline void IdentityConstraint::setNamespaceURI(int uri)
{
    fNamespaceURI = uri;
}

// ---------------------------------------------------------------------------
//  IdentityConstraint: Access methods
// ---------------------------------------------------------------------------
inline void IdentityConstraint::addField(IC_Field* const field) {

    if (!fFields) {
        fFields = new (fMemoryManager) RefVectorOf<IC_Field>(4, true, fMemoryManager);
    }

    fFields->addElement(field);
}

inline const IC_Field* IdentityConstraint::getFieldAt(const unsigned int index) const {

    if (fFields) {
        return (fFields->elementAt(index));
    }

    return 0;
}

inline IC_Field* IdentityConstraint::getFieldAt(const unsigned int index) {

    if (fFields) {
        return (fFields->elementAt(index));
    }

    return 0;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file IdentityConstraint.hpp
  */

