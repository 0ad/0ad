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
 * $Id: BMPattern.hpp,v 1.4 2003/05/15 18:42:54 knoaman Exp $
 */

#if !defined(BMPATTERN_HPP)
#define BMPATTERN_HPP
// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLUTIL_EXPORT BMPattern : public XMemory
{
public:
	// -----------------------------------------------------------------------
	//  Public Constructors and Destructor
    // -----------------------------------------------------------------------
	/** @name Constructors */
    //@{

	/**
      * This is the onstructor which takes the pattern information. A default
      * shift table size is used.
      *
      * @param  pattern     The pattern to match against.
      *
      * @param  ignoreCase  A flag to indicate whether to ignore case
	  *						matching or not.
      *
      * @param  manager     The configurable memory manager
      */
	BMPattern
    (
        const XMLCh* const pattern
        , bool ignoreCase
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

	/**
      * This is the constructor which takes all of the information
      * required to construct a BM pattern object.
      *
      * @param  pattern     The pattern to match against.
      *
	  * @param	tableSize	Indicates the size of the shift table.
	  *
      * @param  ignoreCase  A flag to indicate whether to ignore case
	  *						matching or not.
      *
      * @param  manager     The configurable memory manager
      */
	BMPattern
    (
        const XMLCh* const pattern
        , int tableSize
        , bool ignoreCase
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

	//@}

	/** @name Destructor. */
    //@{

	/**
	  * Destructor of BMPattern
	  */
	~BMPattern();

	//@}

	// -----------------------------------------------------------------------
	// Matching functions
	// -----------------------------------------------------------------------
	/** @name Matching Functions */
	//@{

	/**
	  *	This method will perform a match of the given content against a
	  *	predefined pattern.
	  */
	int matches(const XMLCh* const content, int start, int limit);

	//@}

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    BMPattern();
    BMPattern(const BMPattern&);
    BMPattern& operator=(const BMPattern&);

		// -----------------------------------------------------------------------
	// This method will perform a case insensitive match
	// -----------------------------------------------------------------------
	bool matchesIgnoreCase(const XMLCh ch1, const XMLCh ch2);

	// -----------------------------------------------------------------------
	// Initialize/Clean up methods
	// -----------------------------------------------------------------------
	void initialize();
	void cleanUp();

	// -----------------------------------------------------------------------
    //  Private data members
    //
    //  fPattern
	//	fUppercasePattern
    //      This is the pattern to match against, and its upper case form.
	//		
    //  fIgnoreCase
    //      This is an indicator whether cases should be ignored during
	//		matching.
    //
    //  fShiftTable
	//	fShiftTableLen
    //      This is a table of offsets for shifting purposes used by the BM
	//		search algorithm, and its length.
    // -----------------------------------------------------------------------
	bool           fIgnoreCase;
	unsigned int   fShiftTableLen;
	int*           fShiftTable;
	XMLCh*         fPattern;
	XMLCh*         fUppercasePattern;
    MemoryManager* fMemoryManager; 
};

XERCES_CPP_NAMESPACE_END

#endif

/*
 * End of file BMPattern.hpp
 */

