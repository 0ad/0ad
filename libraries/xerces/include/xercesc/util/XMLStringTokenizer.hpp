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
 * $Id: XMLStringTokenizer.hpp,v 1.6 2004/01/29 11:48:47 cargilld Exp $
 */

#if !defined(XMLSTRINGTOKENIZER_HPP)
#define XMLSTRINGTOKENIZER_HPP

#include <xercesc/util/RefArrayVectorOf.hpp>
#include <xercesc/util/XMLString.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * The string tokenizer class breaks a string into tokens.
  *
  * The XMLStringTokenizer methods do not distinguish among identifiers,
  * numbers, and quoted strings, nor do they recognize and skip comments
  *
  * A XMLStringTokenizer object internally maintains a current position within
  * the string to be tokenized. Some operations advance this current position
  * past the characters processed.
  */


  class XMLUTIL_EXPORT XMLStringTokenizer :public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Public Constructors
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    /**
      * Constructs a string tokenizer for the specified string. The tokenizer
      * uses the default delimiter set, which is "\t\n\r\f": the space
      * character, the tab character, the newline character, the
      * carriage-return character, and the form-feed character. Delimiter
      * characters themselves will not be treated as tokens.
      *
      * @param  srcStr  The string to be parsed.
      * @param  manager Pointer to the memory manager to be used to
      *                 allocate objects.
      *
      */
	XMLStringTokenizer(const XMLCh* const srcStr,
                       MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /**
      * Constructs a string tokenizer for the specified string. The characters
      * in the delim argument are the delimiters for separating tokens.
      * Delimiter characters themselves will not be treated as tokens.
      *
      * @param  srcStr  The string to be parsed.
      * @param  delim   The set of delimiters.
      * @param  manager Pointer to the memory manager to be used to
      *                 allocate objects.
      */
    XMLStringTokenizer(const XMLCh* const srcStr
                       , const XMLCh* const delim
                       , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    //@}

	// -----------------------------------------------------------------------
    //  Public Destructor
    // -----------------------------------------------------------------------
	/** @name Destructor. */
    //@{

    ~XMLStringTokenizer();

    //@}

    // -----------------------------------------------------------------------
    // Management methods
    // -----------------------------------------------------------------------
    /** @name Management Function */
    //@{

     /**
       * Tests if there are more tokens available from this tokenizer's string.
       *
       * Returns true if and only if there is at least one token in the string
       * after the current position; false otherwise.
       */
	bool hasMoreTokens();

    /**
      * Calculates the number of times that this tokenizer's nextToken method
      * can be called to return a valid token. The current position is not
      * advanced.
      *
      * Returns the number of tokens remaining in the string using the current
      * delimiter set.
      */
    int countTokens();

    /**
      * Returns the next token from this string tokenizer.
      *
      * Function allocated, function managed (fafm). The calling function
      * does not need to worry about deleting the returned pointer.
	  */
	XMLCh* nextToken();

    //@}

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLStringTokenizer(const XMLStringTokenizer&);
    XMLStringTokenizer& operator=(const XMLStringTokenizer&);

    // -----------------------------------------------------------------------
    //  CleanUp methods
    // -----------------------------------------------------------------------
	void cleanUp();

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    bool isDelimeter(const XMLCh ch);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fOffset
    //      The current position in the parsed string.
    //
    //  fStringLen
    //      The length of the string parsed (for convenience).
    //
    //  fString
    //      The string to be parsed
	//
    //  fDelimeters
    //      A set of delimeter characters
    //
    //  fTokens
    //      A vector of the token strings
    // -----------------------------------------------------------------------
    int                 fOffset;
    int                 fStringLen;
	XMLCh*              fString;
    XMLCh*              fDelimeters;
	RefArrayVectorOf<XMLCh>* fTokens;
    MemoryManager*           fMemoryManager;
};


// ---------------------------------------------------------------------------
//  XMLStringTokenizer: CleanUp methods
// ---------------------------------------------------------------------------
inline void XMLStringTokenizer::cleanUp() {

	fMemoryManager->deallocate(fString);//delete [] fString;
    fMemoryManager->deallocate(fDelimeters);//delete [] fDelimeters;
    delete fTokens;
}

// ---------------------------------------------------------------------------
//  XMLStringTokenizer: Helper methods
// ---------------------------------------------------------------------------
inline bool XMLStringTokenizer::isDelimeter(const XMLCh ch) {

    return XMLString::indexOf(fDelimeters, ch) == -1 ? false : true;
}


// ---------------------------------------------------------------------------
//  XMLStringTokenizer: Management methods
// ---------------------------------------------------------------------------
inline int XMLStringTokenizer::countTokens() {

    if (fStringLen == 0)
		return 0;

    int  tokCount = 0;
    bool inToken = false;

    for (int i= fOffset; i< fStringLen; i++) {

        if (isDelimeter(fString[i])) {

            if (inToken) {
                inToken = false;
            }

            continue;
        }

		if (!inToken) {

            tokCount++;
            inToken = true;
        }

    } // end for

    return tokCount;
}

XERCES_CPP_NAMESPACE_END

#endif

/**
  * End of file XMLStringTokenizer.hpp
  */

