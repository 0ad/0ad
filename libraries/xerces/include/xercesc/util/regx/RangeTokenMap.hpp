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
 * $Id: RangeTokenMap.hpp,v 1.5 2003/05/15 18:42:54 knoaman Exp $
 */

#if !defined(RANGETOKENMAP_HPP)
#define RANGETOKENMAP_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/Mutexes.hpp>
#include <xercesc/util/RefHashTableOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward Declaration
// ---------------------------------------------------------------------------
class RangeToken;
class RangeFactory;
class TokenFactory;
class XMLStringPool;

class XMLUTIL_EXPORT RangeTokenElemMap : public XMemory
{

public:
    RangeTokenElemMap(unsigned int categoryId);
    ~RangeTokenElemMap();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
	unsigned int getCategoryId() const;
	RangeToken*  getRangeToken(const bool complement = false) const;

	// -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
	void setRangeToken(RangeToken* const tok, const bool complement = false);
	void setCategoryId(const unsigned int categId);

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RangeTokenElemMap(const RangeTokenElemMap&);
    RangeTokenElemMap& operator=(const RangeTokenElemMap&);

    // Data members
    unsigned int fCategoryId;
    RangeToken*  fRange;
    RangeToken*  fNRange;
};


class XMLUTIL_EXPORT RangeTokenMap : public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Putter methods
    // -----------------------------------------------------------------------
    void addCategory(const XMLCh* const categoryName);
	void addRangeMap(const XMLCh* const categoryName,
                     RangeFactory* const rangeFactory);
    void addKeywordMap(const XMLCh* const keyword,
                       const XMLCh* const categoryName);

    // -----------------------------------------------------------------------
    //  Instance methods
    // -----------------------------------------------------------------------
	static RangeTokenMap* instance();

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
	void setRangeToken(const XMLCh* const keyword, RangeToken* const tok,
                       const bool complement = false);

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
	TokenFactory* getTokenFactory() const;

	// -----------------------------------------------------------------------
    //  Notification that lazy data has been deleted
    // -----------------------------------------------------------------------
	static void reinitInstance();

protected:
    // -----------------------------------------------------------------------
    //  Constructor and destructors
    // -----------------------------------------------------------------------
    RangeTokenMap();
    ~RangeTokenMap();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /*
     *  Gets a commonly used RangeToken from the token registry based on the
     *  range name - Called by TokenFactory.
     */
	 RangeToken* getRange(const XMLCh* const name,
                          const bool complement = false);

     RefHashTableOf<RangeTokenElemMap>* getTokenRegistry() const;
     RefHashTableOf<RangeFactory>* getRangeMap() const;
	 XMLStringPool* getCategories() const;

private:
	// -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    RangeTokenMap(const RangeTokenMap&);
    RangeTokenMap& operator=(const RangeTokenMap&);

    // -----------------------------------------------------------------------
    //  Private Helpers methods
    // -----------------------------------------------------------------------
    /*
     *  Initializes the registry with a set of commonly used RangeToken
     *  objects.
     */
     void initializeRegistry();
	 friend class TokenFactory;

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fTokenRegistry
    //      Contains a set of commonly used tokens
	//
    //  fRangeMap
    //      Contains a map between a category name and a RangeFactory object.
    //
    //  fCategories
    //      Contains range categories names
    //
    //  fTokenFactory
    //      Token factory object
    //
    //  fInstance
    //      A RangeTokenMap instance
    //
    //  fMutex
    //      A mutex object for synchronization
    // -----------------------------------------------------------------------
    bool                               fRegistryInitialized;	
    RefHashTableOf<RangeTokenElemMap>* fTokenRegistry;
    RefHashTableOf<RangeFactory>*      fRangeMap;
	XMLStringPool*                     fCategories;
    TokenFactory*                      fTokenFactory;
    XMLMutex                           fMutex;
    static RangeTokenMap*              fInstance;
};

// ---------------------------------------------------------------------------
//  RangeTokenElemMap: Getter methods
// ---------------------------------------------------------------------------
inline unsigned int RangeTokenElemMap::getCategoryId() const {

    return fCategoryId;
}

inline RangeToken* RangeTokenElemMap::getRangeToken(const bool complement) const {

	return complement ? fNRange : fRange;
}

// ---------------------------------------------------------------------------
//  RangeTokenElemMap: Setter methods
// ---------------------------------------------------------------------------
inline void RangeTokenElemMap::setCategoryId(const unsigned int categId) {

    fCategoryId = categId;
}

inline void RangeTokenElemMap::setRangeToken(RangeToken* const tok,
                                      const bool complement) {

    if (complement)
        fNRange = tok;
	else
        fRange = tok;
}

// ---------------------------------------------------------------------------
//  RangeTokenMap: Getter methods
// ---------------------------------------------------------------------------
inline RefHashTableOf<RangeTokenElemMap>* RangeTokenMap::getTokenRegistry() const {

    return fTokenRegistry;
}

inline RefHashTableOf<RangeFactory>* RangeTokenMap::getRangeMap() const {

    return fRangeMap;
}

inline XMLStringPool* RangeTokenMap::getCategories() const {

    return fCategories;
}

inline TokenFactory* RangeTokenMap::getTokenFactory() const {

    return fTokenFactory;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  *	End file RangeToken.hpp
  */
