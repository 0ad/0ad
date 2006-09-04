/*
 * Copyright 1999-2002,2004 The Apache Software Foundation.
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
 * $Id: DOMString.hpp 176234 2004-12-09 18:34:19Z amassari $
 */

#ifndef DOMString_HEADER_GUARD_
#define DOMString_HEADER_GUARD_

#include <xercesc/util/XMemory.hpp>

#ifdef XML_DEBUG
#include "DOMStringImpl.hpp"
XERCES_CPP_NAMESPACE_BEGIN
#else
XERCES_CPP_NAMESPACE_BEGIN
class DOMStringHandle;
#endif

class DOM_NullPtr;

/**
 * <code>DOMString</code> is the generic string class that stores all strings
 * used in the DOM C++ API.
 *
 * Though this class supports most of the common string operations to manipulate
 * strings, it is not meant to be a comphrehensive string class.
 */

class DEPRECATED_DOM_EXPORT DOMString : public XMemory{
public:
    /** @name Constructors and assignment operator */
    //@{
    /**
      * Default constructor for DOMString.  The resulting DOMString
      * object refers to no string at all; it will compare == 0.
      *
      */
    DOMString();

    /**
      * Copy constructor.
      *
      * @param other The object to be copied.
      */
    DOMString(const DOMString &other);

    /**
      * Constructor to build a DOMString from an XML character array.
      * (XMLCh is a 16 bit UNICODE character).
      *
      * @param other The null-terminated character array to be
      *   that provides the initial value for the DOMString.
      */
    DOMString(const XMLCh *other);

    /**
      * Constructor to build a DOMString from a character array of given length.
      *
      * @param other The character array to be imported into the <code>DOMString</code>
      * @param length The length of the character array to be imported
      */
    DOMString(const XMLCh *other, unsigned int length);

    /**
      * Constructor to build a DOMString from an 8 bit character array.
      * The char * string will be transcoded to UNICODE using the default
      * code page on the system where the code is running.
      *
      * @param other The character array to be imported into the <code>DOMString</code>
      */
    DOMString(const char *other);

    /**
      * Construct a null DOMString.
      */
    DOMString(int nullPointerValue);

    /**
      * Assignment operator.  Make destination DOMString refer to the same
      *      underlying string in memory as the source string.
      *
      * @param other the source DOMString.
      */
    DOMString &        operator = (const DOMString &other);



    DOMString &        operator = (DOM_NullPtr *other);

    //@}
    /** @name Destructor. */
    //@{

	 /**
	  * Destructor for DOMString
	  *
	  */
    ~DOMString();

    //@}
    /** @name Operators for string manipulation. */
    //@{

    /**
      * Concatenate a DOMString to another.
      *
      * @param other The string to be concatenated.
      * @return The concatenated object
      */
    // DOMString   operator + (const DOMString &other);

    //@}
    /** @name Equality and Inequality operators. */
    //@{

    /**
      * Equality operator.
      *
      * @param other The object to be compared with.
      * @return True if the two DOMStrings refer to the same underlying string
      *  in memory.
      *  <p>
      *  WARNING:  operator == does NOT compare the contents of
      *  the two  strings.  To do this, use the <code>DOMString::equals()</code>
      *  This behavior is modelled after the String operations in Java, and
      *  is also similar to operator == on the other DOM_* classes.
      */
    bool        operator == (const DOMString &other) const;

    /**
      * Inequality operator.
      *
      * @param other The object to be compared with.
      * @return True if the two DOMStrings refer to different underlying strings in
      *    memory.
      * <p>
      *  WARNING:  operator == does NOT compare the contents of
      *  the two  strings.  To do this, use the <code>DOMString::equals()</code>
      *  This behavior is modelled after the String operations in Java, and
      *  is also similar to operator == on the other DOM_* classes.
      */
    bool        operator != (const DOMString &other) const;

    /**
      * Equality operator.  Test for a null DOMString, which is one that does
      *   not refer to any string at all; similar to a null object reference
      *   variable in Java.
      *
      * @param other must be 0 or null.
      * @return
      */
    bool        operator == (const DOM_NullPtr *other) const;

    /**
      * Inequality operator, for null test.
      *
      * @param other must be 0 or null.
      * @return True if the two strings are different, false otherwise
      */
    bool        operator != (const DOM_NullPtr *other) const;

    //@}
    /** @name Functions to change the string. */
    //@{


    /**
      * Preallocate storage in the string to hold a given number of characters.
      * A DOMString will grow its buffer on demand, as characters are added,
      * but it can be more efficient to allocate once in advance, if the size is known.
      *
      * @param size The number of 16 bit characters to reserve.
      */
    void reserve(unsigned int size);


    /**
      * Appends the content of another <code>DOMString</code> to this string.
      *
      * @param other The object to be appended
      */
    void        appendData(const DOMString &other);

    /**
      * Append a single Unicode character to this string.
      *
      * @param ch The single character to be appended
      */
    void        appendData(XMLCh ch);

     /**
      * Append a null-terminated XMLCh * (Unicode) string to this string.
      *
      * @param other The object to be appended
      */
    void        appendData(const XMLCh *other);


    /**
      * Appends the content of another <code>DOMString</code> to this string.
      *
      * @param other The object to be appended
      */
	DOMString& operator +=(const DOMString &other);


    /**
      * Appends the content of a c-style string to this string.
      *
      * @param other The string to be appended
      */
    DOMString& operator +=(const XMLCh* other);


    /**
      * Appends a character to this string.
      *
      * @param ch The character to be appended
      */
	DOMString& operator +=(XMLCh ch);


    /**
      * Clears the data of this <code>DOMString</code>.
      *
      * @param offset The position from the beginning from which the data must be deleted
      * @param count The count of characters from the offset that must be deleted
      */
    void        deleteData(unsigned int offset, unsigned int count);

    /**
      * Inserts a string within the existing <code>DOMString</code> at an arbitrary position.
      *
      * @param offset The offset from the beginning at which the insertion needs to be done
      *               in <code>this</code> object
      * @param data The <code>DOMString</code> containing the data that needs to be inserted
      * @return The object to be returned.
      */
    void        insertData(unsigned int offset, const DOMString &data);

    //@}
    /** @name Functions to get properties of the string. */
    //@{

    /**
      * Returns the character at the specified position.
      *
      * @param index The position at which the character is being requested
      * @return Returns the character at the specified position.
      */
    XMLCh       charAt(unsigned int index) const;

    /**
      * Returns a handle to the raw buffer in the <code>DOMString</code>.
      *
      * @return The pointer inside the <code>DOMString</code> containg the string data.
      *         Note: the data is not always null terminated.  Do not rely on
      *         a null being there, and do not add one, as several DOMStrings
      *         with different lengths may share the same raw buffer.
      */
    const XMLCh *rawBuffer() const;

    /**
      * Returns a copy of the string, transcoded to the local code page. The
      * caller owns the (char *) string that is returned, and is responsible
      * for deleting it.
      *
      * Note: The buffer returned is allocated using the global operator new
      *       and users need to make sure to use the corresponding delete [].
      *       This method will be deprecated in later versions, as we move
      *       towards using a memory manager for allocation and deallocation.
      *
      * @return A pointer to a newly allocated buffer of char elements, which
      *         represents the original string, but in the local encoding.
      */
    char        *transcode() const;

    /**
      * Returns a copy of the string, transcoded to the local code page. The
      * caller owns the (char *) string that is returned, and is responsible
      * for deleting it.
      *
      * @param  manager the memory manager to use for allocating returned
      *         returned buffer.
      *
      * @return A pointer to a newly allocated buffer of char elements, which
      *         represents the original string, but in the local encoding.
      */
    char        *transcode(MemoryManager* const manager) const;


    /**
      * Creates a DOMString, transcoded from an input 8 bit char * string
      * in the local code page.
      *
      * @param str The string to be transcoded
      * @return A new DOMString object
      */
    static DOMString transcode(const char* str);



    /**
      * Returns a sub-string of the <code>DOMString</code> starting at a specified position.
      *
      * @param offset The offset from the beginning from which the sub-string is being requested.
      * @param count The count of characters in the requested sub-string
      * @return The sub-string of the <code>DOMString</code> being requested
      */
    DOMString   substringData(unsigned int offset, unsigned int count) const;

    /**
      * Returns the length of the DOMString.
      *
      * @return The length of the string
      */
    unsigned int length() const;

    //@}
    /** @name Cloning function. */
    //@{

    /**
      * Makes a clone of a the DOMString.
      *
      * @return The object to be cloned.
      */
    DOMString   clone() const;

    //@}
    /** @name Print functions. */
    //@{

    /**
      * Dumps the <code>DOMString</code> on the console.
      *
      */
    void        print() const;

    /**
      * Dumps the <code>DOMString</code> on the console with a line feed at the end.
      *
      */
    void        println() const;

    //@}
    /** @name Functions to compare a string with another. */
    //@{

    /**
      * Compares a DOMString with another.
      *
      * This compareString does not match the semantics of the standard C strcmp.
      * All it needs to do is define some less than - equals - greater than
      * ordering of strings.  How doesn't matter.
      *
      *
      * @param other The object to be compared with
      * @return Either -1, 0, or 1 based on the comparison.
      */
    int         compareString(const DOMString &other) const;

    /**
      * Less than operator. It is a helper operator for compareString.
      *
      * @param other The object to be compared with.
      * @return True if this DOMString is lexically less than the other DOMString.
      */
    bool        operator < (const DOMString &other) const;

    /**
      * Tells if a <code>DOMString</code> contains the same character data
      * as another.
      *
      * @param other The DOMString to be compared with.
      * @return True if the two <code>DOMString</code>s are same, false otherwise.
      */
    bool        equals(const DOMString &other) const;


      /**
      * Compare a DOMString with a null-terminated raw 16-bit character
      * string.
      *
      * @param other The character string to be compared with.
      * @return True if the strings are the same, false otherwise.
      */
    bool        equals(const XMLCh  *other) const;


    //@}
    friend      class DOMStringData;
    friend      class DOMStringHandle;
    friend      class DomMemDebug;
private:

    DOMStringHandle         *fHandle;
    static int              gLiveStringHandleCount;
    static int              gTotalStringHandleCount;
    static int              gLiveStringDataCount;
    static int              gTotalStringDataCount;
};


/****** Global Helper Functions ******/

/**
  * Concatenate two DOMString's.
  *
  * @param lhs the first string
  * @param rhs the second string
  * @return The concatenated object
  */
DOMString DEPRECATED_DOM_EXPORT operator + (const DOMString &lhs, const DOMString &rhs);

/**
  * Concatenate a null terminated Unicode string to a DOMString.
  *
  * @param lhs the DOMString
  * @param rhs the XMLCh * string
  * @return The concatenated object
  */
DOMString DEPRECATED_DOM_EXPORT operator + (const DOMString &lhs, const XMLCh* rhs);


/**
  * Concatenate a DOMString to a null terminated Unicode string
  *
  * @param lhs the null-terminated Unicode string
  * @param rhs the DOMString
  * @return The concatenated object
  */
DOMString DEPRECATED_DOM_EXPORT operator + (const XMLCh* lhs, const DOMString &rhs);


/**
  * Concatenate a single Unicode character to a DOMString.
  *
  * @param lhs the DOMString
  * @param rhs the character
  * @return The concatenated object
  */
DOMString DEPRECATED_DOM_EXPORT operator + (const DOMString &lhs, XMLCh rhs);


/**
  * Concatenate a DOMString to a single Unicode character.
  *
  * @param lhs the character
  * @param rhs the DOMString
  * @return The concatenated object
  */
DOMString DEPRECATED_DOM_EXPORT operator + (XMLCh lhs, const DOMString &rhs);

XERCES_CPP_NAMESPACE_END

#endif
