/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2003 The Apache Software Foundation.  All rights
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
 * $Log: XMLContentModel.hpp,v $
 * Revision 1.4  2003/05/15 18:26:07  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2003/03/07 18:08:10  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.2  2002/11/04 15:00:21  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:21:51  peiyongz
 * sane_include
 *
 * Revision 1.14  2001/11/21 14:30:13  knoaman
 * Fix for UPA checking.
 *
 * Revision 1.13  2001/08/21 16:06:10  tng
 * Schema: Unique Particle Attribution Constraint Checking.
 *
 * Revision 1.12  2001/08/13 15:06:38  knoaman
 * update <any> validation.
 *
 * Revision 1.11  2001/05/28 20:53:35  tng
 * Schema: move static data gInvalidTrans, gEOCFakeId, gEpsilonFakeId to XMLContentModel for others to access
 *
 * Revision 1.10  2001/05/11 13:25:31  tng
 * Copyright update.
 *
 * Revision 1.9  2001/05/03 21:02:23  tng
 * Schema: Add SubstitutionGroupComparator and update exception messages.  By Pei Yong Zhang.
 *
 * Revision 1.8  2001/04/19 18:16:50  tng
 * Schema: SchemaValidator update, and use QName in Content Model
 *
 * Revision 1.7  2001/03/21 21:56:01  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 * Revision 1.6  2001/02/27 14:48:30  tng
 * Schema: Add CMAny and ContentLeafNameTypeVector, by Pei Yong Zhang
 *
 * Revision 1.5  2000/03/02 19:54:24  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.4  2000/02/24 20:00:23  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.3  2000/02/15 01:21:30  roddey
 * Some initial documentation improvements. More to come...
 *
 * Revision 1.2  2000/02/06 07:47:48  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:08:30  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:44:37  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(CONTENTMODEL_HPP)
#define CONTENTMODEL_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/QName.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class ContentLeafNameTypeVector;
class GrammarResolver;
class XMLStringPool;
class XMLValidator;
class SchemaGrammar;

/**
 *  This class defines the abstract interface for all content models. All
 *  elements have a content model against which (if validating) its content
 *  is checked. Each type of validator (DTD, Schema, etc...) can have
 *  different types of content models, and even with each type of validator
 *  there can be specialized content models. So this simple class provides
 *  the abstract API via which all the types of contents models are dealt
 *  with generically. Its pretty simple.
 */
class XMLPARSER_EXPORT XMLContentModel : public XMemory
{
public:
    // ---------------------------------------------------------------------------
    //  Public static data
    //
    //  gInvalidTrans
    //      This value represents an invalid transition in each line of the
    //      transition table.
    //
    //  gEOCFakeId
    //  gEpsilonFakeId
    //      We have to put in a couple of special CMLeaf nodes to represent
    //      special values, using fake element ids that we know won't conflict
    //      with real element ids.
    //
    //
    // ---------------------------------------------------------------------------
    static const unsigned int   gInvalidTrans;
    static const unsigned int   gEOCFakeId;
    static const unsigned int   gEpsilonFakeId;

    // -----------------------------------------------------------------------
    //  Constructors are hidden, only the virtual Destructor is exposed
    // -----------------------------------------------------------------------
    /** @name Destructor */
    //@{
    virtual ~XMLContentModel()
    {
    }
    //@}


    // -----------------------------------------------------------------------
    //  The virtual content model interface provided by derived classes
    // -----------------------------------------------------------------------
	virtual int validateContent
    (
        QName** const         children
      , const unsigned int    childCount
      , const unsigned int    emptyNamespaceId
    ) const = 0;

	virtual int validateContentSpecial
    (
        QName** const           children
      , const unsigned int      childCount
      , const unsigned int      emptyNamespaceId
      , GrammarResolver*  const pGrammarResolver
      , XMLStringPool*    const pStringPool
    ) const =0;

	virtual void checkUniqueParticleAttribution
    (
        SchemaGrammar*    const pGrammar
      , GrammarResolver*  const pGrammarResolver
      , XMLStringPool*    const pStringPool
      , XMLValidator*     const pValidator
      , unsigned int*     const pContentSpecOrgURI
    ) =0;

    virtual ContentLeafNameTypeVector* getContentLeafNameTypeVector()
	  const = 0;

    virtual unsigned int getNextState(const unsigned int currentState,
                                      const unsigned int elementIndex) const = 0;

protected :
    // -----------------------------------------------------------------------
    //  Hidden Constructors
    // -----------------------------------------------------------------------
    XMLContentModel()
    {
    }


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLContentModel(const XMLContentModel&);
    XMLContentModel& operator=(const XMLContentModel&);
};

XERCES_CPP_NAMESPACE_END

#endif
