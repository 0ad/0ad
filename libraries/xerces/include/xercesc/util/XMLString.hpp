/*
 * Copyright 1999-2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Log$
 * Revision 1.34  2005/04/05 17:00:45  cargilld
 * Implement version of lowercase that only check a to z, instead of all characters, and don't rely on functionality provided in the transcoders.
 *
 * Revision 1.33  2005/03/20 19:02:45  cargilld
 * Implement versions of uppercase and compareIstring that only check a to z, instead of all characters, and don't rely on functionality provided in the transcoders.
 *
 * Revision 1.32  2005/03/08 09:04:09  amassari
 * Improve performances of XMLString::tokenizeString (jira# 1363) - patch by Christian Will
 *
 * Revision 1.31  2004/12/21 16:02:51  cargilld
 * Attempt to fix various apidoc problems.
 *
 * Revision 1.30  2004/12/14 02:09:20  cargilld
 * Performance update from Christian Will.
 *
 * Revision 1.29  2004/12/06 10:47:01  amassari
 * Added XMLString::release(void**, MemoryManager*) [jira# 1301]
 *
 * Revision 1.28  2004/09/08 13:56:24  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.27  2004/09/02 19:08:09  cargilld
 * Fix API Doc warning message
 *
 * Revision 1.26  2004/08/11 16:07:27  peiyongz
 * isValidNOTATION
 *
 * Revision 1.25  2004/05/25 18:11:32  peiyongz
 * removeChar() added
 *
 * Revision 1.24  2004/03/10 17:35:17  amassari
 * Fix documentation for binToText (bug# 9207)
 *
 * Revision 1.23  2003/12/24 15:24:13  cargilld
 * More updates to memory management so that the static memory manager.
 *
 * Revision 1.22  2003/12/17 20:00:49  cargilld
 * Update for memory management so that the static memory manager (one
 * used to call Initialize) is only for static data.
 *
 * Revision 1.21  2003/12/17 00:18:35  cargilld
 * Update to memory management so that the static memory manager (one used to call Initialize) is only for static data.
 *
 * Revision 1.20  2003/10/02 11:07:26  gareth
 * Made the non-memory manager version of replicate not inlined. Updated the documentation for the memory manager versions so they don't tell you you should call release.
 *
 * Revision 1.19  2003/10/01 00:22:20  knoaman
 * Add a lastIndexOf method that takes the string length as one of the params.
 *
 * Revision 1.18  2003/08/25 20:39:47  neilg
 * fix XMLString::findAny(...) docs so that they match what the method actually does (and has done since time immemorial)
 *
 * Revision 1.17  2003/05/18 14:02:05  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.16  2003/05/15 19:07:46  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.15  2003/04/21 20:07:05  knoaman
 * Performance: use memcpy in moveChars and replicate.
 *
 * Revision 1.14  2003/02/25 16:42:31  tng
 * [Bug 7072] Documentation for XMLString::transcode states invalid return value.
 *
 * Revision 1.13  2003/02/05 18:50:56  tng
 * [Bug 11915] Utility for freeing memory.
 *
 * Revision 1.12  2003/01/24 23:16:33  peiyongz
 * removeWS() added;
 *
 * Revision 1.11  2002/12/20 22:10:21  tng
 * XML 1.1
 *
 * Revision 1.10  2002/12/18 14:17:54  gareth
 * Fix to bug #13438. When you eant a vector that calls delete[] on its members you should use RefArrayVectorOf.
 *
 * Revision 1.9  2002/12/04 02:32:43  knoaman
 * #include cleanup.
 *
 * Revision 1.8  2002/11/05 17:42:39  peiyongz
 * equals( const char* const, const char* const)
 *
 * Revision 1.7  2002/11/04 15:22:05  tng
 * C++ Namespace Support.
 *
 * Revision 1.6  2002/10/01 19:45:22  tng
 * Performance in XMLString::equals, only need to check one string for null as they are equal already.
 *
 * Revision 1.5  2002/09/24 19:41:21  tng
 * New inline function equals that is modified from compareString but simply return true or false.
 *
 * Revision 1.4  2002/09/23 18:42:18  tng
 * DOM L3: Support baseURI.   Add utility fixURI to transform an absolute path filename to standard URI form.
 *
 * Revision 1.3  2002/08/27 19:24:43  peiyongz
 * Bug#12087: patch from Thomas Ford (tom@decisionsoft.com)
 *
 * Revision 1.2  2002/02/20 18:17:02  tng
 * [Bug 5977] Warnings on generating apiDocs.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:16  peiyongz
 * sane_include
 *
 * Revision 1.26  2001/08/10 16:23:06  peiyongz
 * isHex(), isAlphaNum(), isAllWhiteSpace() and patternMatch() Added
 *
 * Revision 1.25  2001/07/06 20:27:57  peiyongz
 * isValidaQName()
 *
 * Revision 1.24  2001/07/04 14:38:20  peiyongz
 * IDDatatypeValidator: created
 * DatatypeValidatorFactory: IDDTV enabled
 * XMLString:isValidName(): to validate Name (XML [4][5])
 *
 * Revision 1.23  2001/06/13 14:07:55  peiyongz
 * isValidaEncName() to validate an encoding name (EncName)
 *
 * Revision 1.22  2001/05/23 15:44:51  tng
 * Schema: NormalizedString fix.  By Pei Yong Zhang.
 *
 * Revision 1.21  2001/05/11 13:26:31  tng
 * Copyright update.
 *
 * Revision 1.20  2001/05/09 18:43:30  tng
 * Add StringDatatypeValidator and BooleanDatatypeValidator.  By Pei Yong Zhang.
 *
 * Revision 1.19  2001/05/03 20:34:35  tng
 * Schema: SchemaValidator update
 *
 * Revision 1.18  2001/05/03 19:17:35  knoaman
 * TraverseSchema Part II.
 *
 * Revision 1.17  2001/03/21 21:56:13  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 * Revision 1.16  2001/03/02 20:52:46  knoaman
 * Schema: Regular expression - misc. updates for error messages,
 * and additions of new functions to XMLString class.
 *
 * Revision 1.15  2001/01/15 21:26:34  tng
 * Performance Patches by David Bertoni.
 *
 * Details: (see xerces-c-dev mailing Jan 14)
 * XMLRecognizer.cpp: the internal encoding string XMLUni::fgXMLChEncodingString
 * was going through this function numerous times.  As a result, the top hot-spot
 * for the parse was _wcsicmp().  The real problem is that the Microsofts wide string
 * functions are unbelievably slow.  For things like encodings, it might be
 * better to use a special comparison function that only considers a-z and
 * A-Z as characters with case.  This works since the character set for
 * encodings is limit to printable ASCII characters.
 *
 *  XMLScanner2.cpp: This also has some case-sensitive vs. insensitive compares.
 * They are also much faster.  The other tweak is to only make a copy of an attribute
 * string if it needs to be split.  And then, the strategy is to try to use a
 * stack-based buffer, rather than a dynamically-allocated one.
 *
 * SAX2XMLReaderImpl.cpp: Again, more case-sensitive vs. insensitive comparisons.
 *
 * KVStringPair.cpp & hpp: By storing the size of the allocation, the storage can
 * likely be re-used many times, cutting down on dynamic memory allocations.
 *
 * XMLString.hpp: a more efficient implementation of stringLen().
 *
 * DTDValidator.cpp: another case of using a stack-based buffer when possible
 *
 * These patches made a big difference in parse time in some of our test
 * files, especially the ones are very attribute-heavy.
 *
 * Revision 1.14  2000/10/13 22:47:57  andyh
 * Fix bug (failure to null-terminate result) in XMLString::trim().
 * Patch contributed by Nadav Aharoni
 *
 * Revision 1.13  2000/04/12 18:42:15  roddey
 * Improved docs in terms of what 'max chars' means in the method
 * parameters.
 *
 * Revision 1.12  2000/04/06 19:42:51  rahulj
 * Clarified how big the target buffer should be in the API
 * documentation.
 *
 * Revision 1.11  2000/03/23 01:02:38  roddey
 * Updates to the XMLURL class to correct a lot of parsing problems
 * and to add support for the port number. Updated the URL tests
 * to test some of this new stuff.
 *
 * Revision 1.10  2000/03/20 23:00:46  rahulj
 * Moved the inline definition of stringLen before the first
 * use. This satisfied the HP CC compiler.
 *
 * Revision 1.9  2000/03/02 19:54:49  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.8  2000/02/24 20:05:26  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.7  2000/02/16 18:51:52  roddey
 * Fixed some facts in the docs and reformatted the docs to stay within
 * a reasonable line width.
 *
 * Revision 1.6  2000/02/16 17:07:07  abagchi
 * Added API docs
 *
 * Revision 1.5  2000/02/06 07:48:06  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.4  2000/01/12 00:16:23  roddey
 * Changes to deal with multiply nested, relative pathed, entities and to deal
 * with the new URL class changes.
 *
 * Revision 1.3  1999/12/18 00:18:10  roddey
 * More changes to support the new, completely orthagonal support for
 * intrinsic encodings.
 *
 * Revision 1.2  1999/12/15 19:41:28  roddey
 * Support for the new transcoder system, where even intrinsic encodings are
 * done via the same transcoder abstraction as external ones.
 *
 * Revision 1.1.1.1  1999/11/09 01:05:52  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:21  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#if !defined(XMLSTRING_HPP)
#define XMLSTRING_HPP

#include <xercesc/util/BaseRefVectorOf.hpp>
#include <xercesc/framework/XMLBuffer.hpp>
#include <xercesc/framework/MemoryManager.hpp>
#include <string.h>
#include <assert.h>

XERCES_CPP_NAMESPACE_BEGIN

class XMLLCPTranscoder;
/**
  * Class for representing native character strings and handling common string
  * operations
  *
  * This class is Unicode compliant. This class is designed primarily
  * for internal use, but due to popular demand, it is being made
  * publicly available. Users of this class must understand that this
  * is not an officially supported class. All public methods of this
  * class are <i>static functions</i>.
  *
  */
class XMLUTIL_EXPORT XMLString
{
public:
    /* Static methods for native character mode string manipulation */


    /** @name String concatenation functions */
    //@{
    /** Concatenates two strings.
      *
      * <code>catString</code> appends <code>src</code> to <code>target</code> and
      * terminates the resulting string with a null character. The initial character
      * of <code>src</code> overwrites the terminating character of <code>target
      * </code>.
      *
      * No overflow checking is performed when strings are copied or appended.
      * The behavior of <code>catString</code> is undefined if source and
      * destination strings overlap.
      *
      * @param target Null-terminated destination string
      * @param src Null-terminated source string
      */
    static void catString
    (
                char* const     target
        , const char* const     src
    );

    /** Concatenates two strings.
      *
      * <code>catString</code> appends <code>src</code> to <code>target</code> and
      * terminates the resulting string with a null character. The initial character of
      * <code>src</code> overwrites the terminating character of <code>target</code>.
      * No overflow checking is performed when strings are copied or appended.
      * The behavior of <code>catString</code> is undefined if source and destination
      * strings overlap.
      *
      * @param target Null-terminated destination string
      * @param src Null-terminated source string
      */
    static void catString
    (
                XMLCh* const    target
        , const XMLCh* const    src
    );
    //@}

    /** @name String comparison functions */
    //@{
    /** Lexicographically compares lowercase versions of <code>str1</code> and
      * <code>str2</code> and returns a value indicating their relationship.
      * @param str1 Null-terminated string to compare
      * @param str2 Null-terminated string to compare
      *
      * @return The return value indicates the relation of <code>str1</code> to
      * <code>str2</code> as follows
      *  Less than 0 means <code>str1</code> is less than <code>str2</code>
      *  Equal to 0 means <code>str1</code> is identical to <code>str2</code>
      *  Greater than 0 means <code>str1</code> is more than <code>str2</code>
      */
    static int compareIString
    (
        const   char* const     str1
        , const char* const     str2
    );

    /** Lexicographically compares lowercase versions of <code>str1</code> and
      * <code>str2</code> and returns a value indicating their relationship.
      * @param str1 Null-terminated string to compare
      * @param str2 Null-terminated string to compare
      * @return The return value indicates the relation of <code>str1</code> to
      * <code>str2</code> as follows
      *  Less than 0 means <code>str1</code> is less than <code>str2</code>
      *  Equal to 0 means <code>str1</code> is identical to <code>str2</code>
      *  Greater than 0 means <code>str1</code> is more than <code>str2</code>
      */
    static int compareIString
    (
        const   XMLCh* const    str1
        , const XMLCh* const    str2
    );

    /** Lexicographically compares lowercase versions of <code>str1</code> and
      * <code>str2</code> and returns a value indicating their relationship.
      * The routine only lowercases A to Z.
      * @param str1 Null-terminated ASCII string to compare
      * @param str2 Null-terminated ASCII string to compare
      * @return The return value indicates the relation of <code>str1</code> to
      * <code>str2</code> as follows
      *  Less than 0 means <code>str1</code> is less than <code>str2</code>
      *  Equal to 0 means <code>str1</code> is identical to <code>str2</code>
      *  Greater than 0 means <code>str1</code> is more than <code>str2</code>
      */
    static int compareIStringASCII
    (
        const   XMLCh* const    str1
        , const XMLCh* const    str2
    );



    /** Lexicographically compares, at most, the first count characters in
      * <code>str1</code> and <code>str2</code> and returns a value indicating the
      * relationship between the substrings.
      * @param str1 Null-terminated string to compare
      * @param str2 Null-terminated string to compare
      * @param count The number of characters to compare
      *
      * @return The return value indicates the relation of <code>str1</code> to
      * <code>str2</code> as follows
      *  Less than 0 means <code>str1</code> is less than <code>str2</code>
      *  Equal to 0 means <code>str1</code> is identical to <code>str2</code>
      *  Greater than 0 means <code>str1</code> is more than <code>str2</code>
      */
    static int compareNString
    (
        const   char* const     str1
        , const char* const     str2
        , const unsigned int    count
    );

    /** Lexicographically compares, at most, the first count characters in
      * <code>str1</code> and <code>str2</code> and returns a value indicating
      * the relationship between the substrings.
      * @param str1 Null-terminated string to compare
      * @param str2 Null-terminated string to compare
      * @param count The number of characters to compare
      *
      * @return The return value indicates the relation of <code>str1</code> to
      * <code>str2</code> as follows
      *  Less than 0 means <code>str1</code> is less than <code>str2</code>
      *  Equal to 0 means <code>str1</code> is identical to <code>str2</code>
      *  Greater than 0 means <code>str1</code> is more than <code>str2</code>
      */
    static int compareNString
    (
        const   XMLCh* const    str1
        , const XMLCh* const    str2
        , const unsigned int    count
    );


    /** Lexicographically compares, at most, the first count characters in
      * <code>str1</code> and <code>str2</code> without regard to case and
      * returns a value indicating the relationship between the substrings.
      *
      * @param str1 Null-terminated string to compare
      * @param str2 Null-terminated string to compare
      * @param count The number of characters to compare
      * @return The return value indicates the relation of <code>str1</code> to
      * <code>str2</code> as follows
      *  Less than 0 means <code>str1</code> is less than <code>str2</code>
      *  Equal to 0 means <code>str1</code> is identical to <code>str2</code>
      *  Greater than 0 means <code>str1</code> is more than <code>str2</code>
      */
    static int compareNIString
    (
        const   char* const     str1
        , const char* const     str2
        , const unsigned int    count
    );

    /** Lexicographically compares, at most, the first count characters in
      * <code>str1</code> and <code>str2</code> without regard to case and
      * returns a value indicating the relationship between the substrings.
      *
      * @param str1 Null-terminated string to compare
      * @param str2 Null-terminated string to compare
      * @param count The number of characters to compare
      *
      * @return The return value indicates the relation of <code>str1</code> to
      * <code>str2</code> as follows
      *  Less than 0 means <code>str1</code> is less than <code>str2</code>
      *  Equal to 0 means <code>str1</code> is identical to <code>str2</code>
      *  Greater than 0 means <code>str1</code> is more than <code>str2</code>
      */
    static int compareNIString
    (
        const   XMLCh* const    str1
        , const XMLCh* const    str2
        , const unsigned int    count
    );

    /** Lexicographically compares <code>str1</code> and <code>str2</code> and
      * returns a value indicating their relationship.
      *
      * @param str1 Null-terminated string to compare
      * @param str2 Null-terminated string to compare
      *
      * @return The return value indicates the relation of <code>str1</code> to
      * <code>str2</code> as follows
      *  Less than 0 means <code>str1</code> is less than <code>str2</code>
      *  Equal to 0 means <code>str1</code> is identical to <code>str2</code>
      *  Greater than 0 means <code>str1</code> is more than <code>str2</code>
      */
    static int compareString
    (
        const   char* const     str1
        , const char* const     str2
    );

    /** Lexicographically compares <code>str1</code> and <code>str2</code> and
      * returns a value indicating their relationship.
      *
      * @param str1 Null-terminated string to compare
      * @param str2 Null-terminated string to compare
      * @return The return value indicates the relation of <code>str1</code> to
      * <code>str2</code> as follows
      *  Less than 0 means <code>str1</code> is less than <code>str2</code>
      *  Equal to 0 means <code>str1</code> is identical to <code>str2</code>
      *  Greater than 0 means <code>str1</code> is more than <code>str2</code>
      */
    static int compareString
    (
        const   XMLCh* const    str1
        , const XMLCh* const    str2
    );

    /** compares <code>str1</code> and <code>str2</code>
      *
      * @param str1 Null-terminated string to compare
      * @param str2 Null-terminated string to compare
      * @return true if two strings are equal, false if not
      *  If one string is null, while the other is zero-length string,
      *  it is considered as equal.
      */
    static bool equals
    (
          const XMLCh* const    str1
        , const XMLCh* const    str2
    );

    static bool equals
    (
          const char* const    str1
        , const char* const    str2
    );

	/** Lexicographically compares <code>str1</code> and <code>str2</code>
	  * regions and returns true if they are equal, otherwise false.
	  *
      * A substring of <code>str1</code> is compared to a substring of
	  * <code>str2</code>. The result is true if these substrings represent
	  * identical character sequences. The substring of <code>str1</code>
      * to be compared begins at offset1 and has length charCount. The
	  * substring of <code>str2</code> to be compared begins at offset2 and
	  * has length charCount. The result is false if and only if at least
      * one of the following is true:
      *   offset1 is negative.
      *   offset2 is negative.
      *   offset1+charCount is greater than the length of str1.
      *   offset2+charCount is greater than the length of str2.
      *   There is some nonnegative integer k less than charCount such that:
      *   str1.charAt(offset1+k) != str2.charAt(offset2+k)
      *
      * @param str1 Null-terminated string to compare
	  * @param offset1 Starting offset of str1
      * @param str2 Null-terminated string to compare
	  * @param offset2 Starting offset of str2
	  * @param charCount The number of characters to compare
      * @return true if the specified subregion of <code>str1</code> exactly
	  *  matches the specified subregion of <code>str2></code>; false
	  *  otherwise.
      */
    static bool regionMatches
    (
        const   XMLCh* const    str1
		, const	int				offset1
        , const XMLCh* const    str2
		, const int				offset2
		, const unsigned int	charCount
    );

	/** Lexicographically compares <code>str1</code> and <code>str2</code>
	  * regions without regard to case and returns true if they are equal,
	  * otherwise false.
	  *
      * A substring of <code>str1</code> is compared to a substring of
	  * <code>str2</code>. The result is true if these substrings represent
	  * identical character sequences. The substring of <code>str1</code>
      * to be compared begins at offset1 and has length charCount. The
	  * substring of <code>str2</code> to be compared begins at offset2 and
	  * has length charCount. The result is false if and only if at least
      * one of the following is true:
      *   offset1 is negative.
      *   offset2 is negative.
      *   offset1+charCount is greater than the length of str1.
      *   offset2+charCount is greater than the length of str2.
      *   There is some nonnegative integer k less than charCount such that:
      *   str1.charAt(offset1+k) != str2.charAt(offset2+k)
      *
      * @param str1 Null-terminated string to compare
	  * @param offset1 Starting offset of str1
      * @param str2 Null-terminated string to compare
	  * @param offset2 Starting offset of str2
	  * @param charCount The number of characters to compare
      * @return true if the specified subregion of <code>str1</code> exactly
	  *  matches the specified subregion of <code>str2></code>; false
	  *  otherwise.
      */
    static bool regionIMatches
    (
        const   XMLCh* const    str1
		, const	int				offset1
        , const XMLCh* const    str2
		, const int				offset2
		, const unsigned int	charCount
    );
    //@}

    /** @name String copy functions */
    //@{
    /** Copies <code>src</code>, including the terminating null character, to the
      * location specified by <code>target</code>.
      *
      * No overflow checking is performed when strings are copied or appended.
      * The behavior of strcpy is undefined if the source and destination strings
      * overlap.
      *
      * @param target Destination string
      * @param src Null-terminated source string
      */
    static void copyString
    (
                char* const     target
        , const char* const     src
    );

    /** Copies <code>src</code>, including the terminating null character, to
      *   the location specified by <code>target</code>.
      *
      * No overflow checking is performed when strings are copied or appended.
      * The behavior of <code>copyString</code> is undefined if the source and
      * destination strings overlap.
      *
      * @param target Destination string
      * @param src Null-terminated source string
      */
    static void copyString
    (
                XMLCh* const    target
        , const XMLCh* const    src
    );

    /** Copies <code>src</code>, upto a fixed number of characters, to the
      * location specified by <code>target</code>.
      *
      * No overflow checking is performed when strings are copied or appended.
      * The behavior of <code>copyNString</code> is undefined if the source and
      * destination strings overlap.
      *
      * @param target Destination string. The size of the buffer should
      *        atleast be 'maxChars + 1'.
      * @param src Null-terminated source string
      * @param maxChars The maximum number of characters to copy
      */
    static bool copyNString
    (
                XMLCh* const    target
        , const XMLCh* const    src
        , const unsigned int    maxChars
    );
    //@}

    /** @name Hash functions */
    //@{
    /** Hashes a string given a modulus
      *
      * @param toHash The string to hash
      * @param hashModulus The divisor to be used for hashing
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns the hash value
      */
    static unsigned int hash
    (
        const   char* const     toHash
        , const unsigned int    hashModulus
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Hashes a string given a modulus
      *
      * @param toHash The string to hash
      * @param hashModulus The divisor to be used for hashing
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns the hash value
      */
    static unsigned int hash
    (
        const   XMLCh* const    toHash
        , const unsigned int    hashModulus
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Hashes a string given a modulus taking a maximum number of characters
      * as the limit
      *
      * @param toHash The string to hash
      * @param numChars The maximum number of characters to consider for hashing
      * @param hashModulus The divisor to be used for hashing
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns the hash value
      */
    static unsigned int hashN
    (
        const   XMLCh* const    toHash
        , const unsigned int    numChars
        , const unsigned int    hashModulus
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    //@}

    /** @name Search functions */
    //@{
    /**
      * Provides the index of the first occurance of a character within a string
      *
      * @param toSearch The string to search
      * @param ch The character to search within the string
      * @return If found, returns the index of the character within the string,
      * else returns -1.
      */
    static int indexOf(const char* const toSearch, const char ch);

    /**
      * Provides the index of the first occurance of a character within a string
      *
      * @param toSearch The string to search
      * @param ch The character to search within the string
      * @return If found, returns the index of the character within the string,
      * else returns -1.
      */
    static int indexOf(const XMLCh* const toSearch, const XMLCh ch);

	    /**
      * Provides the index of the first occurance of a character within a string
      * starting from a given index
      *
      * @param toSearch The string to search
      * @param chToFind The character to search within the string
      * @param fromIndex The index to start searching from
      * @param manager The MemoryManager to use to allocate objects
      * @return If found, returns the index of the character within the string,
      * else returns -1.
      */
    static int indexOf
    (
        const   char* const     toSearch
        , const char            chToFind
        , const unsigned int    fromIndex
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /**
      * Provides the index of the first occurance of a character within a string
      * starting from a given index
      *
      * @param toSearch The string to search
      * @param chToFind The character to search within the string
      * @param fromIndex The index to start searching from
      * @param manager The MemoryManager to use to allocate objects
      * @return If found, returns the index of the character within the string,
      * else returns -1.
      */
    static int indexOf
    (
        const   XMLCh* const    toSearch
        , const XMLCh           chToFind
        , const unsigned int    fromIndex
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /**
      * Provides the index of the last occurance of a character within a string
      *
      * @param toSearch The string to search
      * @param ch The character to search within the string
      * @return If found, returns the index of the character within the string,
      * else returns -1.
      */
    static int lastIndexOf(const char* const toSearch, const char ch);

    /**
      * Provides the index of the last occurance of a character within a string
      *
      * @param toSearch The string to search
      * @param ch The character to search within the string
      * @return If found, returns the index of the character within the string,
      * else returns -1.
      */
    static int lastIndexOf(const XMLCh* const toSearch, const XMLCh ch);

    /**
      * Provides the index of the last occurance of a character within a string
      *
      * @param ch The character to search within the string
      * @param toSearch The string to search
      * @param toSearchLen The length of the string to search
      * @return If found, returns the index of the character within the string,
      * else returns -1.
      */
    static int lastIndexOf
    (
        const XMLCh ch
        , const XMLCh* const toSearch
        , const unsigned int toSearchLen
    );

    /**
      * Provides the index of the last occurance of a character within a string
      * starting backward from a given index
      *
      * @param toSearch The string to search
      * @param chToFind The character to search within the string
      * @param fromIndex The index to start backward search from
      * @param manager The MemoryManager to use to allocate objects
      * @return If found, returns the index of the character within the string,
      * else returns -1.
      */
    static int lastIndexOf
    (
        const   char* const     toSearch
        , const char            chToFind
        , const unsigned int    fromIndex
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /**
      * Provides the index of the last occurance of a character within a string
      * starting backward from a given index
      *
      * @param toSearch The string to search
      * @param ch       The character to search within the string
      * @param fromIndex The index to start backward search from
      * @param manager The MemoryManager to use to allocate objects
      * @return If found, returns the index of the character within the string,
      * else returns -1.
      */
    static int lastIndexOf
    (
        const   XMLCh* const    toSearch
        , const XMLCh           ch
        , const unsigned int    fromIndex
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );
    //@}

    /** @name Fixed size string movement */
    //@{
    /** Moves X number of chars
      * @param targetStr The string to copy the chars to
      * @param srcStr The string to copy the chars from
      * @param count The number of chars to move
      */
    static void moveChars
    (
                XMLCh* const    targetStr
        , const XMLCh* const    srcStr
        , const unsigned int    count
    );

    //@}

    /** @name Substring function */
    //@{
    /** Create a substring of a given string. The substring begins at the
      * specified beginIndex and extends to the character at index
      * endIndex - 1.
      * @param targetStr The string to copy the chars to
      * @param srcStr The string to copy the chars from
      * @param startIndex beginning index, inclusive.
      * @param endIndex the ending index, exclusive.
      * @param manager The MemoryManager to use to allocate objects
      */
    static void subString
    (
                char* const    targetStr
        , const char* const    srcStr
        , const int            startIndex
        , const int            endIndex
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Create a substring of a given string. The substring begins at the
      * specified beginIndex and extends to the character at index
      * endIndex - 1.
      * @param targetStr The string to copy the chars to
      * @param srcStr The string to copy the chars from
      * @param startIndex beginning index, inclusive.
      * @param endIndex the ending index, exclusive.
      * @param manager The MemoryManager to use to allocate objects
      */
    static void subString
    (
                XMLCh* const    targetStr
        , const XMLCh* const    srcStr
        , const int             startIndex
        , const int             endIndex
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Create a substring of a given string. The substring begins at the
      * specified beginIndex and extends to the character at index
      * endIndex - 1.
      * @param targetStr The string to copy the chars to
      * @param srcStr The string to copy the chars from
      * @param startIndex beginning index, inclusive.
      * @param endIndex the ending index, exclusive.
      * @param srcStrLength the length of srcStr
      * @param manager The MemoryManager to use to allocate objects
      */
    static void subString
    (
                XMLCh* const    targetStr
        , const XMLCh* const    srcStr
        , const int             startIndex
        , const int             endIndex
        , const int             srcStrLength
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    //@}

    /** @name Replication function */
    //@{
    /** Replicates a string
      * NOTE: The returned buffer is dynamically allocated and is the
      * responsibility of the caller to delete it when not longer needed.
      * You can call XMLString::release to release this returned buffer.
      *
      * @param toRep The string to replicate
      * @return Returns a pointer to the replicated string
      * @see   XMLString::release(char**)
      */
    static char* replicate(const char* const toRep);

    /** Replicates a string
      * NOTE: The returned buffer is allocated with the MemoryManager. It is the
      * responsibility of the caller to delete it when not longer needed.
      *
      * @param toRep The string to replicate
      * @param manager The MemoryManager to use to allocate the string
      * @return Returns a pointer to the replicated string
      */
    static char* replicate(const char* const toRep,
                           MemoryManager* const manager);

    /** Replicates a string
      * NOTE: The returned buffer is dynamically allocated and is the
      * responsibility of the caller to delete it when not longer needed.
      * You can call XMLString::release to release this returned buffer.

      * @param toRep The string to replicate
      * @return Returns a pointer to the replicated string
      * @see   XMLString::release(XMLCh**)
      */
    static XMLCh* replicate(const XMLCh* const toRep);

    /** Replicates a string
      * NOTE: The returned buffer is allocated with the MemoryManager. It is the
      * responsibility of the caller to delete it when not longer needed.
      *
      * @param toRep The string to replicate
      * @param manager The MemoryManager to use to allocate the string
      * @return Returns a pointer to the replicated string
      */
    static XMLCh* replicate(const XMLCh* const toRep,
                            MemoryManager* const manager);

    //@}

    /** @name String query function */
    //@{
    /** Tells if the sub-string appears within a string at the beginning
      * @param toTest The string to test
      * @param prefix The sub-string that needs to be checked
      * @return Returns true if the sub-string was found at the beginning of
      * <code>toTest</code>, else false
      */
    static bool startsWith
    (
        const   char* const     toTest
        , const char* const     prefix
    );

    /** Tells if the sub-string appears within a string at the beginning
      * @param toTest The string to test
      * @param prefix The sub-string that needs to be checked
      * @return Returns true if the sub-string was found at the beginning of
      * <code>toTest</code>, else false
      */
    static bool startsWith
    (
        const   XMLCh* const    toTest
        , const XMLCh* const    prefix
    );

    /** Tells if the sub-string appears within a string at the beginning
      * without regard to case
      *
      * @param toTest The string to test
      * @param prefix The sub-string that needs to be checked
      * @return Returns true if the sub-string was found at the beginning of
      * <code>toTest</code>, else false
      */
    static bool startsWithI
    (
        const   char* const     toTest
        , const char* const     prefix
    );

    /** Tells if the sub-string appears within a string at the beginning
      * without regard to case
      *
      * @param toTest The string to test
      * @param prefix The sub-string that needs to be checked
      *
      * @return Returns true if the sub-string was found at the beginning
      * of <code>toTest</code>, else false
      */
    static bool startsWithI
    (
        const   XMLCh* const    toTest
        , const XMLCh* const    prefix
    );

    /** Tells if the sub-string appears within a string at the end.
      * @param toTest The string to test
      * @param suffix The sub-string that needs to be checked
      * @return Returns true if the sub-string was found at the end of
      * <code>toTest</code>, else false
      */
    static bool endsWith
    (
        const   XMLCh* const    toTest
        , const XMLCh* const    suffix
    );


    /** Tells if a string has any occurance of any character of another 
      * string within itself
      * @param toSearch The string to be searched
      * @param searchList The string from which characters to be searched for are drawn 
      * @return Returns the pointer to the location where the first occurrence of any
      * character from searchList is found,
      * else returns 0
      */
    static const XMLCh* findAny
    (
        const   XMLCh* const    toSearch
        , const XMLCh* const    searchList
    );

    /** Tells if a string has any occurance of any character of another 
      * string within itself
      * @param toSearch The string to be searched
      * @param searchList The string from which characters to be searched for are drawn 
      * @return Returns the pointer to the location where the first occurrence of any
      * character from searchList is found,
      * else returns 0
      */
    static XMLCh* findAny
    (
                XMLCh* const    toSearch
        , const XMLCh* const    searchList
    );

    /** Tells if a string has pattern within itself
      * @param toSearch The string to be searched
      * @param pattern The pattern to be located within the string
      * @return Returns index to the location where the pattern was
      * found, else returns -1
      */
    static int patternMatch
    (
          const XMLCh* const    toSearch
        , const XMLCh* const    pattern
    );

    /** Get the length of the string
      * @param src The string whose length is to be determined
      * @return Returns the length of the string
      */
    static unsigned int stringLen(const char* const src);

    /** Get the length of the string
      * @param src The string whose length is to be determined
      * @return Returns the length of the string
      */
    static unsigned int stringLen(const XMLCh* const src);

    /**
      *
      * Checks whether an name is a valid NOTATION according to XML 1.0
      * @param name    The string to check its NOTATION validity
      * @param manager The memory manager
      * @return Returns true if name is NOTATION valid, otherwise false
      */
    static bool isValidNOTATION(const XMLCh*         const name
                              ,       MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /**
      * Deprecated: please use XMLChar1_0::isValidNCName
      *
      * Checks whether an name is a valid NCName according to XML 1.0
      * @param name The string to check its NCName validity
      * @return Returns true if name is NCName valid, otherwise false
      */
    static bool isValidNCName(const XMLCh* const name);

    /**
      * Deprecated: please use XMLChar1_0::isValidName
      *
      * Checks whether an name is a valid Name according to XML 1.0
      * @param name The string to check its Name validity
      * @return Returns true if name is Name valid, otherwise false
      */
    static bool isValidName(const XMLCh* const name);

    /**
      * Checks whether an name is a valid EncName.
      * @param name The string to check its EncName validity
      * @return Returns true if name is EncName valid, otherwise false
      */
    static bool isValidEncName(const XMLCh* const name);

    /**
      * Deprecated: please use XMLChar1_0::isValidQName
      *
      * Checks whether an name is a valid QName according to XML 1.0
      * @param name The string to check its QName validity
      * @return Returns true if name is QName valid, otherwise false
      */
    static bool isValidQName(const XMLCh* const name);

    /**
      * Checks whether a character is within [a-zA-Z].
      * @param theChar the character to check
      * @return Returns true if within the range, otherwise false
      */

    static bool isAlpha(XMLCh const theChar);

    /**
      * Checks whether a character is within [0-9].
      * @param theChar the character to check
      * @return Returns true if within the range, otherwise false
      */
    static bool isDigit(XMLCh const theChar);

    /**
      * Checks whether a character is within [0-9a-zA-Z].
      * @param theChar the character to check
      * @return Returns true if within the range, otherwise false
      */
    static bool isAlphaNum(XMLCh const theChar);

    /**
      * Checks whether a character is within [0-9a-fA-F].
      * @param theChar the character to check
      * @return Returns true if within the range, otherwise false
      */
    static bool isHex(XMLCh const theChar);

    /**
      * Deprecated: please use XMLChar1_0::isAllWhiteSpace
      *
      * Checks whether aa string contains only whitespace according to XML 1.0
      * @param toCheck the string to check
      * @return Returns true if it is, otherwise false
      */
    static bool isAllWhiteSpace(const XMLCh* const toCheck);

    /** Find is the string appears in the enum list
      * @param toFind the string to be found
      * @param enumList the list
      * return true if found
      */
    static bool isInList(const XMLCh* const toFind, const XMLCh* const enumList);

    //@}

    /** @name Conversion functions */
    //@{

    /** Converts binary data to a text string based a given radix
      *
      * @param toFormat The number to convert
      * @param toFill The buffer that will hold the output on return. The
      *        size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @param radix The radix of the input data, based on which the conversion
      * @param manager The MemoryManager to use to allocate objects
      * will be done
      */
    static void binToText
    (
        const   unsigned int    toFormat
        ,       char* const     toFill
        , const unsigned int    maxChars
        , const unsigned int    radix
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Converts binary data to a text string based a given radix
      *
      * @param toFormat The number to convert
      * @param toFill The buffer that will hold the output on return. The
      *        size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @param radix The radix of the input data, based on which the conversion
      * @param manager The MemoryManager to use to allocate objects
      * will be done
      */
    static void binToText
    (
        const   unsigned int    toFormat
        ,       XMLCh* const    toFill
        , const unsigned int    maxChars
        , const unsigned int    radix
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Converts binary data to a text string based a given radix
      *
      * @param toFormat The number to convert
      * @param toFill The buffer that will hold the output on return. The
      *        size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @param radix The radix of the input data, based on which the conversion
      * @param manager The MemoryManager to use to allocate objects
      * will be done
      */
    static void binToText
    (
        const   unsigned long   toFormat
        ,       char* const     toFill
        , const unsigned int    maxChars
        , const unsigned int    radix
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Converts binary data to a text string based a given radix
      *
      * @param toFormat The number to convert
      * @param toFill The buffer that will hold the output on return. The
      *        size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @param radix The radix of the input data, based on which the conversion
      * @param manager The MemoryManager to use to allocate objects
      * will be done
      */
    static void binToText
    (
        const   unsigned long   toFormat
        ,       XMLCh* const    toFill
        , const unsigned int    maxChars
        , const unsigned int    radix
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Converts binary data to a text string based a given radix
      *
      * @param toFormat The number to convert
      * @param toFill The buffer that will hold the output on return. The
      *        size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @param radix The radix of the input data, based on which the conversion
      * @param manager The MemoryManager to use to allocate objects
      * will be done
      */
    static void binToText
    (
        const   long            toFormat
        ,       char* const     toFill
        , const unsigned int    maxChars
        , const unsigned int    radix
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Converts binary data to a text string based a given radix
      *
      * @param toFormat The number to convert
      * @param toFill The buffer that will hold the output on return. The
      *        size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @param radix The radix of the input data, based on which the conversion
      * @param manager The MemoryManager to use to allocate objects
      * will be done
      */
    static void binToText
    (
        const   long            toFormat
        ,       XMLCh* const    toFill
        , const unsigned int    maxChars
        , const unsigned int    radix
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Converts binary data to a text string based a given radix
      *
      * @param toFormat The number to convert
      * @param toFill The buffer that will hold the output on return. The
      *        size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @param radix The radix of the input data, based on which the conversion
      * @param manager The MemoryManager to use to allocate objects
      * will be done
      */
    static void binToText
    (
        const   int             toFormat
        ,       char* const     toFill
        , const unsigned int    maxChars
        , const unsigned int    radix
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Converts binary data to a text string based a given radix
      *
      * @param toFormat The number to convert
      * @param toFill The buffer that will hold the output on return. The
      *        size of this buffer should at least be 'maxChars + 1'.
      * @param maxChars The maximum number of output characters that can be
      *         accepted. If the result will not fit, it is an error.
      * @param radix The radix of the input data, based on which the conversion
      * @param manager The MemoryManager to use to allocate objects
      * will be done
      */
    static void binToText
    (
        const   int             toFormat
        ,       XMLCh* const    toFill
        , const unsigned int    maxChars
        , const unsigned int    radix
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /**
      * Converts a string of decimal chars to a binary value
      *
      * Note that leading and trailng whitespace is legal and will be ignored
      * but the remainder must be all decimal digits.
      *
      * @param toConvert The string of digits to convert
      * @param toFill    The unsigned int value to fill with the converted
      *                  value.
      * @param manager The MemoryManager to use to allocate objects
      */
    static bool textToBin
    (
        const   XMLCh* const    toConvert
        ,       unsigned int&   toFill
        ,       MemoryManager*  const manager = XMLPlatformUtils::fgMemoryManager
    );

    /**
      * Converts a string of decimal chars to a binary value
      *
      * Note that leading and trailng whitespace is legal and will be ignored,
      *
      * Only one and either of (+,-) after the leading whitespace, before
      * any other characters are allowed.
      *
      * but the remainder must be all decimal digits.
      *
      * @param toConvert The string of digits to convert
      * @param manager The MemoryManager to use to allocate objects
      */
    static int parseInt
    (
        const   XMLCh* const    toConvert
      , MemoryManager* const    manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Cut leading chars from a string
      *
      * @param toCutFrom The string to cut chars from
      * @param count     The count of leading chars to cut
      */
    static void cut
    (
                XMLCh* const    toCutFrom
        , const unsigned int    count
    );

    /** Transcodes a string to native code-page
      *
      * NOTE: The returned buffer is dynamically allocated and is the
      * responsibility of the caller to delete it when not longer needed.
      * You can call XMLString::release to release this returned buffer.
      *
      * @param toTranscode The string to be transcoded      
      * @return Returns the transcoded string
      * @see   XMLString::release(XMLCh**)
      */
    static char* transcode
    (
        const   XMLCh* const    toTranscode
    );
    static char* transcode
    (
        const   XMLCh* const         toTranscode
        ,       MemoryManager* const manager
    );

    /** Transcodes a string to native code-page
      *
      * Be aware that when transcoding to an external encoding, that each
      * Unicode char can create multiple output bytes. So you cannot assume
      * a one to one correspondence of input chars to output bytes.
      *
      * @param toTranscode The string tobe transcoded
      * @param toFill The buffer that is filled with the transcoded value.
      *        The size of this buffer should atleast be 'maxChars + 1'.
      * @param maxChars The maximum number of bytes that the output
      *         buffer can hold (not including the null, which is why
      *         toFill should be at least maxChars+1.).
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns true if successful, false if there was an error
      */
    static bool transcode
    (
        const   XMLCh* const    toTranscode
        ,       char* const     toFill
        , const unsigned int    maxChars
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Transcodes a string to native code-page
      *
      * NOTE: The returned buffer is dynamically allocated and is the
      * responsibility of the caller to delete it when not longer needed.
      * You can call XMLString::release to release this returned buffer.
      *
      * @param toTranscode The string to be transcoded     
      * @return Returns the transcoded string
      * @see   XMLString::release(char**)
      */
    static XMLCh* transcode
    (
        const   char* const     toTranscode
    );
    static XMLCh* transcode
    (
        const   char* const          toTranscode
        ,       MemoryManager* const manager
    );

    /** Transcodes a string to native code-page
      * @param toTranscode The string tobe transcoded
      * @param toFill The buffer that is filled with the transcoded value.
      *        The size of this buffer should atleast be 'maxChars + 1'.
      * @param maxChars The maximum number of characters that the output
      *         buffer can hold (not including the null, which is why
      *         toFill should be at least maxChars+1.).
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns true if successful, false if there was an error
      */
    static bool transcode
    (
        const   char* const     toTranscode
        ,       XMLCh* const    toFill
        , const unsigned int    maxChars
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Trims off extra space characters from the start and end of the string,
      * moving the non-space string content back to the start.
      * @param toTrim The string to be trimmed. On return this contains the
      * trimmed string
      */
    static void trim(char* const toTrim);

    /** Trims off extra space characters from the start and end of the string,
      * moving the non-space string content back to the start.
      * @param toTrim The string to be trimmed. On return this contains
      * the trimmed string
      */
    static void trim(XMLCh* const toTrim);

    /** Break a string into tokens with space as delimiter, and
      * stored in a string vector.  The caller owns the string vector
      * that is returned, and is responsible for deleting it.
      * @param tokenizeSrc String to be tokenized
      * @param manager The MemoryManager to use to allocate objects
      * @return a vector of all the tokenized string
      */
    static BaseRefVectorOf<XMLCh>* tokenizeString(const XMLCh* const tokenizeSrc
                                        , MemoryManager*       const manager = XMLPlatformUtils::fgMemoryManager);

    //@}

    /** @name Formatting functions */
    //@{
    /** Creates a UName from a URI and base name. It is in the form
      * {url}name, and is commonly used internally to represent fully
      * qualified names when namespaces are enabled.
      *
      * @param pszURI The URI part of the name
      * @param pszName The base part of the name
      * @return Returns the complete formatted UName
      */
    static XMLCh* makeUName
    (
        const   XMLCh* const    pszURI
        , const XMLCh* const    pszName      
    );

    /**
      * Internal function to perform token replacement for strings.
      *
      * @param errText The text (NULL terminated) where the replacement
      *        is to be done. The size of this buffer should be
      *        'maxChars + 1' to account for the final NULL.
      * @param maxChars The size of the output buffer, i.e. the maximum
      *         number of characters that it will hold. If the result is
      *         larger, it will be truncated.
      * @param text1 Replacement text-one
      * @param text2 Replacement text-two
      * @param text3 Replacement text-three
      * @param text4 Replacement text-four
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns the count of characters that are outputted
      */
    static unsigned int replaceTokens
    (
                XMLCh* const    errText
        , const unsigned int    maxChars
        , const XMLCh* const    text1
        , const XMLCh* const    text2
        , const XMLCh* const    text3
        , const XMLCh* const    text4
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Converts a string to uppercase
      * @param toUpperCase The string which needs to be converted to uppercase.
      *        On return, this buffer also holds the converted uppercase string
      */
    static void upperCase(XMLCh* const toUpperCase);

    /** Converts a string to uppercase
      * The routine only uppercases A to Z (other characters not changed).
      * @param toUpperCase The string which needs to be converted to uppercase.
      *        On return, this buffer also holds the converted uppercase string
      */
    static void upperCaseASCII(XMLCh* const toUpperCase);

	/** Converts a string to lowercase
      * @param toLowerCase The string which needs to be converted to lowercase.
      *        On return, this buffer also holds the converted lowercase string
      */
    static void lowerCase(XMLCh* const toLowerCase);

    /** Converts a string to lowercase
      * The routine only lowercases a to z (other characters not changed).
      * @param toLowerCase The string which needs to be converted to lowercase.
      *        On return, this buffer also holds the converted lowercase string
      */
    static void lowerCaseASCII(XMLCh* const toLowerCase);

	/** Check if string is WhiteSpace:replace
      * @param toCheck The string which needs to be checked.
      */
    static bool isWSReplaced(const XMLCh* const toCheck);

	/** Check if string is WhiteSpace:collapse
      * @param toCheck The string which needs to be checked.
      */
    static bool isWSCollapsed(const XMLCh* const toCheck);

	/** Replace whitespace
      * @param toConvert The string which needs to be whitespace replaced.
      *        On return , this buffer also holds the converted string
      * @param manager The MemoryManager to use to allocate objects
      */
    static void replaceWS(XMLCh* const toConvert
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager);

	/** Collapse whitespace
      * @param toConvert The string which needs to be whitespace collapsed.
      *        On return , this buffer also holds the converted string
      * @param manager The MemoryManager to use to allocate objects
      */
    static void collapseWS(XMLCh* const toConvert
        , MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager);

    /** Remove whitespace
      * @param toConvert The string which needs to be whitespace removed.
      *        On return , this buffer also holds the converted string
      * @param manager The MemoryManager to use to allocate objects
      */
    static void removeWS(XMLCh* const toConvert
    , MemoryManager*       const manager = XMLPlatformUtils::fgMemoryManager);


    /** Remove character
      * @param srcString The string 
      * @param toRemove  The character needs to be removed from the string
      * @param dstBuffer The buffer containning the result
      */
    static void removeChar(const XMLCh*     const srcString
                         , const XMLCh&           toRemove
                         ,       XMLBuffer&       dstBuffer);

    /**
     * Fixes a platform dependent absolute path filename to standard URI form.
     * 1. Windows: fix 'x:' to 'file:///x:' and convert any backslash to forward slash
     * 2. UNIX: fix '/blah/blahblah' to 'file:///blah/blahblah'
     * @param str    The string that has the absolute path filename
     * @param target The target string pre-allocated to store the fixed uri
     */
    static void fixURI(const XMLCh* const str, XMLCh* const target);

    //@}
    /** @name String Memory Management functions */
    //@{
    /**
     * Release the parameter char string that was allocated by the implementation (i.e.the parser).
     *   The implementation will call operator delete[] and then turn the string to a null pointer.
     *
     * @param buf  The string to be deleted and become a null pointer.
     */
    static void release(char** buf);

    /**
     * Release the parameter XMLCh string that was allocated by the implementation (i.e.the parser).
     *   The implementation will call operator delete[] and then turn the string to a null pointer.
     *
     * @param buf  The string to be deleted and become a null pointer.
     */
    static void release(XMLCh** buf);

    /**
     * Release the parameter XMLByte string that was allocated by the implementation (i.e.the parser).
     *   The implementation will call operator delete[] and then turn the string to a null pointer.
     *
     * @param buf  The string to be deleted and become a null pointer.
     */
    static void release(XMLByte** buf);

    /**
     * Release the parameter string that was allocated using the version of XMLString::transcode
     * that accepts a MemoryManager.
     * The implementation will call MemoryManager::deallocate and then turn the string to a null pointer.
     *
     * @param buf  The string to be deleted and become a null pointer.
     * @param manager The MemoryManager to use to allocate objects
     */
    static void release
    (
        void**  buf
        ,       MemoryManager* const manager
    );

    //@}


private :

    /** @name Constructors and Destructor */
    //@{
    /** Unimplemented default constructor */
    XMLString();
    /** Unimplemented destructor */
    ~XMLString();
    //@}


    /** @name Initialization */
    //@{
    /** Init/Term methods called from XMLPlatformUtils class */
    static void initString(XMLLCPTranscoder* const defToUse,
                           MemoryManager* const manager);
    static void termString();
    //@}

	/**
	  * Called by regionMatches/regionIMatches to validate that we
	  * have a valid input
	  */
	static bool validateRegion(const XMLCh* const str1, const int offset1,
						const XMLCh* const str2, const int offset2,
						const unsigned int charCount);

    static MemoryManager* fgMemoryManager;

    friend class XMLPlatformUtils;
};


// ---------------------------------------------------------------------------
//  Inline some methods that are either just passthroughs to other string
//  methods, or which are key for performance.
// ---------------------------------------------------------------------------
inline void XMLString::moveChars(       XMLCh* const targetStr
                                , const XMLCh* const srcStr
                                , const unsigned int count)
{
    memcpy(targetStr, srcStr, count * sizeof(XMLCh));
}

inline unsigned int XMLString::stringLen(const XMLCh* const src)
{
    if (src == 0 || *src == 0)
    {
        return 0;
   }
    else
   {
        const XMLCh* pszTmp = src + 1;

        while (*pszTmp)
            ++pszTmp;

        return (unsigned int)(pszTmp - src);
    }
}

inline XMLCh* XMLString::replicate(const XMLCh* const toRep,
                                   MemoryManager* const manager)
{
    // If a null string, return a null string!
    XMLCh* ret = 0;
    if (toRep)
    {
        const unsigned int len = stringLen(toRep);
        ret = (XMLCh*) manager->allocate((len+1) * sizeof(XMLCh)); //new XMLCh[len + 1];
        memcpy(ret, toRep, (len + 1) * sizeof(XMLCh));
    }
    return ret;
}

inline bool XMLString::startsWith(  const   XMLCh* const    toTest
                                    , const XMLCh* const    prefix)
{
    return (compareNString(toTest, prefix, stringLen(prefix)) == 0);
}

inline bool XMLString::startsWithI( const   XMLCh* const    toTest
                                    , const XMLCh* const    prefix)
{
    return (compareNIString(toTest, prefix, stringLen(prefix)) == 0);
}

inline bool XMLString::endsWith(const XMLCh* const toTest,
                                const XMLCh* const suffix)
{

    unsigned int suffixLen = XMLString::stringLen(suffix);

    return regionMatches(toTest, XMLString::stringLen(toTest) - suffixLen,
                         suffix, 0, suffixLen);
}

inline bool XMLString::validateRegion(const XMLCh* const str1,
									  const int offset1,
									  const XMLCh* const str2,
									  const int offset2,
									  const unsigned int charCount)
{

	if (offset1 < 0 || offset2 < 0 ||
		(offset1 + charCount) > XMLString::stringLen(str1) ||
		(offset2 + charCount) > XMLString::stringLen(str2) )
		return false;

	return true;
}

inline bool XMLString::equals(   const XMLCh* const    str1
                               , const XMLCh* const    str2)
{
    const XMLCh* psz1 = str1;
    const XMLCh* psz2 = str2;

    if (psz1 == 0 || psz2 == 0) {
        if ((psz1 != 0 && *psz1) || (psz2 != 0 && *psz2))
            return false;
        else
            return true;
    }

    while (*psz1 == *psz2)
    {
        // If either has ended, then they both ended, so equal
        if (!*psz1)
            return true;

        // Move upwards for the next round
        psz1++;
        psz2++;
    }
    return false;
}

inline bool XMLString::equals(   const char* const    str1
                               , const char* const    str2)
{
    const char* psz1 = str1;
    const char* psz2 = str2;

    if (psz1 == 0 || psz2 == 0) {
        if ((psz1 != 0 && *psz1) || (psz2 != 0 && *psz2))
            return false;
        else
            return true;
    }

    while (*psz1 == *psz2)
    {
        // If either has ended, then they both ended, so equal
        if (!*psz1)
            return true;

        // Move upwards for the next round
        psz1++;
        psz2++;
    }
    return false;
}

inline int XMLString::lastIndexOf(const XMLCh* const toSearch, const XMLCh ch)
{
    return XMLString::lastIndexOf(ch, toSearch, stringLen(toSearch));
}

inline unsigned int XMLString::hash(   const   XMLCh* const    tohash
                                , const unsigned int    hashModulus
                                , MemoryManager* const)
{  
    assert(hashModulus);

    if (tohash == 0 || *tohash == 0)
        return 0;

    const XMLCh* curCh = tohash;
    unsigned int hashVal = (unsigned int)(*curCh);
    curCh++;

    while (*curCh)
    {
        hashVal = (hashVal * 38) + (hashVal >> 24) + (unsigned int)(*curCh);
        curCh++;
    }

    // Divide by modulus
    return hashVal % hashModulus;
}

XERCES_CPP_NAMESPACE_END

#endif
