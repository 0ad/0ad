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
 * $Id: Match.hpp,v 1.7 2003/12/17 00:18:37 cargilld Exp $
 */

#if !defined(MATCH_HPP)
#define MATCH_HPP

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/ArrayIndexOutOfBoundsException.hpp>
#include <xercesc/util/RuntimeException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * An instance of this class has ranges captured in matching
  */
  class XMLUTIL_EXPORT Match : public XMemory
{
public:

	// -----------------------------------------------------------------------
  //  Public Constructors and Destructor
  // -----------------------------------------------------------------------
	Match(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
	
  /**
  * Copy constructor
  */
  Match(const Match& toCopy);
  Match& operator=(const Match& toAssign);

	virtual ~Match();

	// -----------------------------------------------------------------------
	// Getter functions
	// -----------------------------------------------------------------------
	int getNoGroups() const;
	int getStartPos(int index) const;
	int getEndPos(int index) const;

	// -----------------------------------------------------------------------
	// Setter functions
	// -----------------------------------------------------------------------
	void setNoGroups(const int n);
	void setStartPos(const int index, const int value);
	void setEndPos(const int index, const int value);

private:
	// -----------------------------------------------------------------------
	// Initialize/Clean up methods
	// -----------------------------------------------------------------------
  void initialize(const Match& toCopy);
	void cleanUp();

	// -----------------------------------------------------------------------
    //  Private data members
    //
    //  fNoGroups
    //  Represents no of regular expression groups
	//		
    //  fStartPositions
    //  Array of start positions in the target text matched to specific
	//		regular expression group
	//
	//	fEndPositions
	//		Array of end positions in the target text matched to specific
	//		regular expression group
	//
	//	fPositionsSize
	//		Actual size of Start/EndPositions array.
    // -----------------------------------------------------------------------
	int fNoGroups;
	int fPositionsSize;
	int* fStartPositions;
	int* fEndPositions;
    MemoryManager* fMemoryManager;
};

/**
  * Inline Methods
  */

// ---------------------------------------------------------------------------
//  Match: getter methods
// ---------------------------------------------------------------------------
inline int Match::getNoGroups() const {

	if (fNoGroups < 0)
		ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_Result_Not_Set, fMemoryManager);

	return fNoGroups;
}

inline int Match::getStartPos(int index) const {

	if (!fStartPositions)
		ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_Result_Not_Set, fMemoryManager);

	if (index < 0 || fNoGroups <= index)
		ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Array_BadIndex, fMemoryManager);

	return fStartPositions[index];
}

inline int Match::getEndPos(int index) const {

	if (!fEndPositions)
		ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_Result_Not_Set, fMemoryManager);

	if (index < 0 || fNoGroups <= index)
		ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Array_BadIndex, fMemoryManager);

	return fEndPositions[index];
}

// ---------------------------------------------------------------------------
//  Match: setter methods
// ---------------------------------------------------------------------------
inline void Match::setStartPos(const int index, const int value) {

	if (!fStartPositions)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_Result_Not_Set, fMemoryManager);

	if (index < 0 || fNoGroups <= index)
		ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Array_BadIndex, fMemoryManager);

	fStartPositions[index] = value;
}

inline void Match::setEndPos(const int index, const int value) {

	if (!fEndPositions)
        ThrowXMLwithMemMgr(RuntimeException, XMLExcepts::Regex_Result_Not_Set, fMemoryManager);

	if (index < 0 || fNoGroups <= index)
		ThrowXMLwithMemMgr(ArrayIndexOutOfBoundsException, XMLExcepts::Array_BadIndex, fMemoryManager);

	fEndPositions[index] = value;
}

XERCES_CPP_NAMESPACE_END

#endif
