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
 * $Id: AbstractStringValidator.hpp,v 1.13 2004/01/29 11:51:22 cargilld Exp $
 * $Log: AbstractStringValidator.hpp,v $
 * Revision 1.13  2004/01/29 11:51:22  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.12  2003/12/31 10:38:00  amassari
 * Made virtual function checkAdditionalFacet 'const', so that it matches the declaration in a derived class
 *
 * Revision 1.11  2003/12/17 00:18:38  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.10  2003/11/12 20:32:03  peiyongz
 * Statless Grammar: ValidationContext
 *
 * Revision 1.9  2003/09/29 21:47:35  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.8  2003/05/15 18:53:26  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.7  2003/01/27 19:24:17  peiyongz
 * normalize Base64 data before checking against enumeration.
 *
 * Revision 1.6  2003/01/24 23:18:34  peiyongz
 * normalizeEnumeration() added to remove optional ws in Base64 data.
 *
 * Revision 1.5  2002/12/18 14:17:55  gareth
 * Fix to bug #13438. When you eant a vector that calls delete[] on its members you should use RefArrayVectorOf.
 *
 * Revision 1.4  2002/11/04 14:53:27  tng
 * C++ Namespace Support.
 *
 * Revision 1.3  2002/10/18 16:52:14  peiyongz
 * Patch to Bug#13640: Getter methods not public in
 *                                    DecimalDatatypeValidator
 *
 * Revision 1.2  2002/02/14 15:17:31  peiyongz
 * getEnumString()
 *
 * Revision 1.1.1.1  2002/02/01 22:22:40  peiyongz
 * sane_include
 *
 * Revision 1.9  2001/12/13 16:48:29  peiyongz
 * Avoid dangling pointer
 *
 * Revision 1.8  2001/11/22 20:23:20  peiyongz
 * _declspec(dllimport) and inline warning C4273
 *
 * Revision 1.7  2001/11/15 16:08:15  peiyongz
 * checkContent() made virtual to allow ListDTV participate in the building of
 * its own (in AbstractStringValidator's init())
 *
 * Revision 1.6  2001/10/11 19:32:12  peiyongz
 * Allow derived to overwrite inheritFacet()
 *
 * Revision 1.5  2001/10/09 21:00:54  peiyongz
 * . init() take 1 arg,
 * . make inspectFacetBase() virtual to allow ListDTV provide its own method,
 * . reorganize init() into assignFacet(), inspectFacet(), inspectFacetBase() and
 * inheritFacet() to improve mantainability.
 * . macro to simplify code
 * . save get***() to temp vars
 *
 * Revision 1.4  2001/09/27 13:51:25  peiyongz
 * DTV Reorganization: ctor/init created to be used by derived class
 *
 * Revision 1.3  2001/09/24 15:30:16  peiyongz
 * DTV Reorganization: init() to be invoked from derived class' ctor to allow
 *        correct resolution of virtual methods like assignAdditionalFacet(),
 *        inheritAdditionalFacet(), etc.
 *
 * Revision 1.2  2001/09/19 18:48:27  peiyongz
 * DTV reorganization:getLength() added, move inline to class declaration to avoid inline
 * function interdependency.
 *
 * Revision 1.1  2001/09/18 14:45:04  peiyongz
 * DTV reorganization
 *
 */

#if !defined(ABSTRACT_STRING_VALIDATOR_HPP)
#define ABSTRACT_STRING_VALIDATOR_HPP

#include <xercesc/validators/datatype/DatatypeValidator.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class VALIDATORS_EXPORT AbstractStringValidator : public DatatypeValidator
{
public:

    // -----------------------------------------------------------------------
    //  Public ctor/dtor
    // -----------------------------------------------------------------------
	/** @name Constructor. */
    //@{

    virtual ~AbstractStringValidator();

	//@}

	virtual const RefArrayVectorOf<XMLCh>* getEnumString() const;

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

    virtual int compare(const XMLCh* const, const XMLCh* const
        ,       MemoryManager*     const manager = XMLPlatformUtils::fgMemoryManager
        );

    //@}

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(AbstractStringValidator)

protected:

    AbstractStringValidator
    (
        DatatypeValidator* const baseValidator
        , RefHashTableOf<KVStringPair>* const facets
        , const int finalSet
        , const ValidatorType type
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    void init(RefArrayVectorOf<XMLCh>*           const enums
        , MemoryManager* const manager);

    //
    // Abstract interface
    //
    virtual void assignAdditionalFacet(const XMLCh* const key
                                     , const XMLCh* const value
                                     , MemoryManager* const manager);

    virtual void inheritAdditionalFacet();

    virtual void checkAdditionalFacetConstraints(MemoryManager* const manager) const;

    virtual void checkAdditionalFacet(const XMLCh* const content
                                    , MemoryManager* const manager) const;

    virtual int  getLength(const XMLCh* const content
        , MemoryManager* const manager) const;
    
    virtual void checkValueSpace(const XMLCh* const content
        , MemoryManager* const manager) = 0;

    //
    //   to Allow ListDTV to overwrite
    //
    virtual void inspectFacetBase(MemoryManager* const manager);

    virtual void inheritFacet();

    virtual void checkContent(const XMLCh*             const content
                            ,       ValidationContext* const context
                            , bool                           asBase
                            , MemoryManager* const manager);

    /*
     **  Base64BinaryDatatypeValidator to overwrite
     */
    virtual void normalizeEnumeration(MemoryManager* const manager);

    virtual void normalizeContent(XMLCh* const, MemoryManager* const manager) const;

public:
// -----------------------------------------------------------------------
// Getter methods
// -----------------------------------------------------------------------

    inline unsigned int         getLength() const;

    inline unsigned int         getMaxLength() const;

    inline unsigned int         getMinLength() const;

    inline RefArrayVectorOf<XMLCh>*  getEnumeration() const;

protected:
// -----------------------------------------------------------------------
// Setter methods
// -----------------------------------------------------------------------

    inline void                 setLength(unsigned int);

    inline void                 setMaxLength(unsigned int);

    inline void                 setMinLength(unsigned int);

    inline void                 setEnumeration(RefArrayVectorOf<XMLCh>*, bool);

private:

    void assignFacet(MemoryManager* const manager);

    void inspectFacet(MemoryManager* const manager);

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    AbstractStringValidator(const AbstractStringValidator&);
    AbstractStringValidator& operator=(const AbstractStringValidator&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    // -----------------------------------------------------------------------
     unsigned int         fLength;
     unsigned int         fMaxLength;
     unsigned int         fMinLength;
     bool                 fEnumerationInherited;
     RefArrayVectorOf<XMLCh>*  fEnumeration;
};

// -----------------------------------------------------------------------
// Getter methods
// -----------------------------------------------------------------------

inline unsigned int AbstractStringValidator::getLength() const
{
    return fLength;
}

inline unsigned int AbstractStringValidator::getMaxLength() const
{
    return fMaxLength;
}

inline unsigned int AbstractStringValidator::getMinLength() const
{
    return fMinLength;
}

inline RefArrayVectorOf<XMLCh>* AbstractStringValidator:: getEnumeration() const
{
    return fEnumeration;
}

// -----------------------------------------------------------------------
// Setter methods
// -----------------------------------------------------------------------

inline void AbstractStringValidator::setLength(unsigned int newLength)
{
    fLength = newLength;
}

inline void AbstractStringValidator::setMaxLength(unsigned int newMaxLength)
{
    fMaxLength = newMaxLength;
}

inline void AbstractStringValidator::setMinLength(unsigned int newMinLength)
{
    fMinLength = newMinLength;
}

inline void AbstractStringValidator::setEnumeration(RefArrayVectorOf<XMLCh>* enums
                                           , bool                inherited)
{
    if (enums)
    {
        if ( !fEnumerationInherited && fEnumeration)
            delete fEnumeration;

        fEnumeration = enums;
        fEnumerationInherited = inherited;
        setFacetsDefined(DatatypeValidator::FACET_ENUMERATION);
    }
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file AbstractStringValidator.hpp
  */
