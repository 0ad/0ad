/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
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
 * $Log: XSParticle.hpp,v $
 * Revision 1.5  2003/12/01 23:23:26  neilg
 * fix for bug 25118; thanks to Jeroen Witmond
 *
 * Revision 1.4  2003/11/21 17:34:04  knoaman
 * PSVI update
 *
 * Revision 1.3  2003/11/14 22:47:53  neilg
 * fix bogus log message from previous commit...
 *
 * Revision 1.2  2003/11/14 22:33:30  neilg
 * Second phase of schema component model implementation.  
 * Implement XSModel, XSNamespaceItem, and the plumbing necessary
 * to connect them to the other components.
 * Thanks to David Cargill.
 *
 * Revision 1.1  2003/09/16 14:33:36  neilg
 * PSVI/schema component model classes, with Makefile/configuration changes necessary to build them
 *
 */

#if !defined(XSPARTICLE_HPP)
#define XSPARTICLE_HPP

#include <xercesc/framework/psvi/XSObject.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * This class describes all properties of a Schema Particle
 * component.
 * This is *always* owned by the validator /parser object from which
 * it is obtained.  
 */

// forward declarations
class XSElementDeclaration;
class XSModelGroup;
class XSWildcard;

class XMLPARSER_EXPORT XSParticle : public XSObject
{
public:

    // possible terms of this particle
    enum TERM_TYPE {
        /*
         * an empty particle
         */
        TERM_EMPTY          = 0,
        /*
         * the particle has element content
         */
        TERM_ELEMENT        = XSConstants::ELEMENT_DECLARATION,
        /*
         * the particle's content is a model group 
         */
        TERM_MODELGROUP     = XSConstants::MODEL_GROUP_DEFINITION,
        /*
         * the particle's content is a wildcard
         */
        TERM_WILDCARD       = XSConstants::WILDCARD
    };

    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    /**
      * The default constructor 
      *
      * @param  termType
      * @param  xsModel
      * @param  particleTerm
      * @param  minOccurs
      * @param  maxOccurs
      * @param  manager     The configurable memory manager
      */
    XSParticle
    (
        TERM_TYPE              termType
        , XSModel* const       xsModel
        , XSObject* const      particleTerm
        , int                  minOccurs
        , int                  maxOccurs
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    //@};

    /** @name Destructor */
    //@{
    ~XSParticle();
    //@}

    //---------------------
    /** @name XSParticle methods */
    //@{

    /**
     * [min occurs]: determines the minimum number of terms that can occur. 
     */
    int getMinOccurs() const;

    /**
     * [max occurs] determines the maximum number of terms that can occur. To 
     * query for value of unbounded use <code>maxOccursUnbounded</code>. 
     */
    int getMaxOccurs() const;

    /**
     * [max occurs] whether the maxOccurs value is unbounded.
     */
    bool getMaxOccursUnbounded() const;

    /**
     * Returns the type of the [term]: one of 
     * TERM_EMPTY, TERM_ELEMENT, TERM_MODELGROUP, or TERM_WILDCARD.
     */
    TERM_TYPE getTermType() const;

    /**
     * If this particle has an [element declaration] for its term,
     * this method returns that declaration; otherwise, it returns 0.
     * @returns The element declaration that is the [term] of this Particle
     * if and only if getTermType() == TERM_ELEMENT.
     */ 
    XSElementDeclaration *getElementTerm();

    /**
     * If this particle has a [model group] for its term,
     * this method returns that definition; otherwise, it returns 0.
     * @returns The model group that is the [term] of this Particle
     * if and only if getTermType() == TERM_MODELGROUP.
     */ 
    XSModelGroup *getModelGroupTerm();

    /**
     * If this particle has an [wildcard] for its term,
     * this method returns that declaration; otherwise, it returns 0.
     * @returns The wildcard declaration that is the [term] of this Particle
     * if and only if getTermType() == TERM_WILDCARD.
     */ 
    XSWildcard *getWildcardTerm();

    //@}

    //----------------------------------
    /** methods needed by implementation */
    //@{

    //@}
private:

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XSParticle(const XSParticle&);
    XSParticle & operator=(const XSParticle &);

protected:

    // -----------------------------------------------------------------------
    //  data members
    // -----------------------------------------------------------------------
    TERM_TYPE fTermType;
    int       fMinOccurs;
    int       fMaxOccurs;
    XSObject* fTerm;
};

inline int XSParticle::getMinOccurs() const
{
    return fMinOccurs;
}

inline int XSParticle::getMaxOccurs() const
{
    return fMaxOccurs;
}

inline bool XSParticle::getMaxOccursUnbounded() const
{
    return (fMaxOccurs == -1);
}

inline XSParticle::TERM_TYPE XSParticle::getTermType() const
{
    return fTermType;
}

XERCES_CPP_NAMESPACE_END

#endif
