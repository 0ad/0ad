/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * $Id: ListDatatypeValidator.hpp,v 1.12 2004/01/29 11:51:22 cargilld Exp $
 * $Log: ListDatatypeValidator.hpp,v $
 * Revision 1.12  2004/01/29 11:51:22  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.11  2003/12/23 21:50:36  peiyongz
 * Absorb exception thrown in getCanonicalRepresentation and return 0,
 * only validate when required
 *
 * Revision 1.10  2003/12/17 00:18:39  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.9  2003/11/28 18:53:07  peiyongz
 * Support for getCanonicalRepresentation
 *
 * Revision 1.8  2003/11/12 20:32:03  peiyongz
 * Statless Grammar: ValidationContext
 *
 * Revision 1.7  2003/09/30 21:31:30  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.6  2003/05/15 18:53:26  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2002/12/18 14:17:55  gareth
 * Fix to bug #13438. When you eant a vector that calls delete[] on its members you should use RefArrayVectorOf.
 *
 * Revision 1.4  2002/12/16 22:28:25  knoaman
 * Make isAtomic inline.
 *
 * Revision 1.3  2002/11/20 19:57:44  peiyongz
 * validate content as a whole against pattern.
 *
 * Revision 1.2  2002/11/04 14:53:28  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:42  peiyongz
 * sane_include
 *
 * Revision 1.11  2001/11/22 20:23:20  peiyongz
 * _declspec(dllimport) and inline warning C4273
 *
 * Revision 1.10  2001/11/15 16:08:15  peiyongz
 * checkContent() made virtual to allow ListDTV participate in the building of
 * its own (in AbstractStringValidator's init())
 *
 * Revision 1.9  2001/10/11 19:32:27  peiyongz
 * To overwrite inheritFacet()
 *
 * Revision 1.8  2001/10/09 20:56:21  peiyongz
 * inherit from AbstractStringValidator instead of DatatypeValidator to reuse
 * the code.
 *
 * Revision 1.7  2001/08/29 19:03:40  peiyongz
 * Bugzilla# 2816:on AIX 4.2, xlC 3 r ev.1, Compilation error on inline method
 *
 * Revision 1.6  2001/08/24 17:12:01  knoaman
 * Add support for anySimpleType.
 * Remove parameter 'baseValidator' from the virtual method 'newInstance'.
 *
 * Revision 1.5  2001/08/21 20:05:41  peiyongz
 * put back changes introduced in 1.3
 *
 * Revision 1.3  2001/08/16 14:41:38  knoaman
 * implementation of virtual methods.
 *
 * Revision 1.2  2001/07/24 21:23:40  tng
 * Schema: Use DatatypeValidator for ID/IDREF/ENTITY/ENTITIES/NOTATION.
 *
 * Revision 1.1  2001/07/11 21:37:17  peiyongz
 * ListDatatypeDTV
 *
 */

#if !defined(LIST_DATATYPEVALIDATOR_HPP)
#define LIST_DATATYPEVALIDATOR_HPP

#include <xercesc/validators/datatype/AbstractStringValidator.hpp>
#include <xercesc/validators/schema/SchemaSymbols.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT ListDatatypeValidator : public AbstractStringValidator
{
public:

    // -----------------------------------------------------------------------
    //  Public ctor/dtor
    // -----------------------------------------------------------------------
	/** @name Constructors and Destructor */
    //@{

    ListDatatypeValidator
    (
        MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ListDatatypeValidator
    (
        DatatypeValidator* const baseValidator
        , RefHashTableOf<KVStringPair>* const facets
        , RefArrayVectorOf<XMLCh>* const enums
        , const int finalSet
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    virtual ~ListDatatypeValidator();

	//@}

    /** @name Getter Functions */
    //@{
    /**
      * Returns whether the type is atomic or not
      */
    virtual bool isAtomic() const;

    virtual const XMLCh* getCanonicalRepresentation
                        (
                          const XMLCh*         const rawData
                        ,       MemoryManager* const memMgr = 0
                        ,       bool                 toValidate = false
                        ) const;

    //@}

    // -----------------------------------------------------------------------
    // Validation methods
    // -----------------------------------------------------------------------
    /** @name Validation Function */
    //@{

    /**
     * validate that a string matches the boolean datatype
     * @param content A string containing the content to be validated
     *
     * @exception throws InvalidDatatypeException if the content is
     * is not valid.
     */

	virtual void validate
                 (
                  const XMLCh*             const content
                ,       ValidationContext* const context = 0
                ,       MemoryManager*     const manager = XMLPlatformUtils::fgMemoryManager
                  );

    //@}

    // -----------------------------------------------------------------------
    // Compare methods
    // -----------------------------------------------------------------------
    /** @name Compare Function */
    //@{

    /**
     * Compare two boolean data types
     *
     * @param content1
     * @param content2
     * @return
     */
    int compare(const XMLCh* const, const XMLCh* const
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
        );

    //@}

    /**
      * Returns an instance of the base datatype validator class
	  * Used by the DatatypeValidatorFactory.
      */
    virtual DatatypeValidator* newInstance
    (
        RefHashTableOf<KVStringPair>* const facets
        , RefArrayVectorOf<XMLCh>* const enums
        , const int finalSet
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    DatatypeValidator* getItemTypeDTV() const;

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(ListDatatypeValidator)

protected:

    //
    // ctor provided to be used by derived classes: No
    //

    virtual void checkValueSpace(const XMLCh* const content
                                , MemoryManager* const manager);

    virtual int  getLength(const XMLCh* const content
            , MemoryManager* const manager) const;

    //
    // Overwrite AbstractStringValidator's
    //
    virtual void inspectFacetBase(MemoryManager* const manager);

    virtual void inheritFacet();

    virtual void checkContent(const XMLCh*             const content
                            ,       ValidationContext* const context
                            , bool                           asBase
                            ,       MemoryManager*     const manager);

private:

    void checkContent(      BaseRefVectorOf<XMLCh>*        tokenVector
                    , const XMLCh*                  const  content
                    ,       ValidationContext*      const  context
                    ,       bool                           asBase
                    ,       MemoryManager*          const  manager
                    );

    bool valueSpaceCheck(BaseRefVectorOf<XMLCh>* tokenVector
                       , const XMLCh*   const enumStr
                       , MemoryManager* const manager) const;

// -----------------------------------------------------------------------
// Getter methods
// -----------------------------------------------------------------------

    inline const XMLCh* const   getContent() const;

// -----------------------------------------------------------------------
// Setter methods
// -----------------------------------------------------------------------

    inline void                 setContent(const XMLCh* const content);

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ListDatatypeValidator(const ListDatatypeValidator&);
    ListDatatypeValidator& operator=(const ListDatatypeValidator&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fContent
    //      temporary var referenceing the content to be validated,
    //      for error reporting purpose.
    //
    // -----------------------------------------------------------------------
     const XMLCh*         fContent;
};

// -----------------------------------------------------------------------
// Getter methods
// -----------------------------------------------------------------------
inline const XMLCh* const ListDatatypeValidator::getContent() const
{
    return fContent;
}

inline bool ListDatatypeValidator::isAtomic() const
{
    return false;
}

// -----------------------------------------------------------------------
// Setter methods
// -----------------------------------------------------------------------
inline void ListDatatypeValidator::setContent(const XMLCh* const content)
{
    fContent = content;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file ListDatatypeValidator.hpp
  */
