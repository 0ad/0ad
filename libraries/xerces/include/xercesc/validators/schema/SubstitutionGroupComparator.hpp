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
 * $Id: SubstitutionGroupComparator.hpp,v 1.4 2004/01/29 11:52:31 cargilld Exp $
 */

#if !defined(SUBSTITUTIONGROUPCOMPARATOR_HPP)
#define SUBSTITUTIONGROUPCOMPARATOR_HPP

#include <xercesc/util/StringPool.hpp>
#include <xercesc/util/QName.hpp>
#include <xercesc/validators/common/GrammarResolver.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class SchemaGrammar;

class VALIDATORS_EXPORT SubstitutionGroupComparator : public XMemory
{
public:

    // -----------------------------------------------------------------------
    //  Public Constructor
    // -----------------------------------------------------------------------
    /** @name Constructor. */
    //@{

    SubstitutionGroupComparator(GrammarResolver*  const pGrammarResolver
                              , XMLStringPool*    const pStringPool);


    //@}

    // -----------------------------------------------------------------------
    //  Public Destructor
    // -----------------------------------------------------------------------
    /** @name Destructor. */
    //@{

    ~SubstitutionGroupComparator();

    //@}

    // -----------------------------------------------------------------------
    // Validation methods
    // -----------------------------------------------------------------------
    /** @name Validation Function */
    //@{

    /**
	   * Checks that the "anElement" is within the subsitution group.
	   *
	   * @param  anElement   QName of the element
	   *
	   * @param  exeplar     QName of the head element in the group
	   */
    bool isEquivalentTo(QName* const anElement
                       , QName* const exemplar);
	 //@}

    /*
     * check whether one element or any element in its substitution group
     * is allowed by a given wildcard uri
     *
     * @param pGrammar the grammar where the wildcard is declared
     * @param element  the QName of a given element
     * @param wuri     the uri of the wildcard
     * @param wother   whether the uri is from ##other, so wuri is excluded
     *
     * @return whether the element is allowed by the wildcard
     */
    bool isAllowedByWildcard(SchemaGrammar* const pGrammar, QName* const element, unsigned int wuri, bool wother);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    SubstitutionGroupComparator();
    SubstitutionGroupComparator(const SubstitutionGroupComparator&);
    SubstitutionGroupComparator& operator=(const SubstitutionGroupComparator&);
    
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //
    // -----------------------------------------------------------------------
    GrammarResolver     *fGrammarResolver;
    XMLStringPool       *fStringPool;
};


// ---------------------------------------------------------------------------
//  SubstitutionGroupComparator: Getters
// ---------------------------------------------------------------------------
inline SubstitutionGroupComparator::SubstitutionGroupComparator(GrammarResolver*  const pGrammarResolver
                                                              , XMLStringPool*    const pStringPool)
:fGrammarResolver(pGrammarResolver)
,fStringPool(pStringPool)
{}

inline SubstitutionGroupComparator::~SubstitutionGroupComparator()
{}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file SubstitutionGroupComparator.hpp
  */

