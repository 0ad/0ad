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
 * $Log: KVStringPair.hpp,v $
 * Revision 1.7  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.6  2003/09/25 22:23:25  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.5  2003/05/18 14:02:05  knoaman
 * Memory manager implementation: pass per instance manager.
 *
 * Revision 1.4  2003/05/16 06:01:52  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2003/05/15 19:04:35  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.2  2002/11/04 15:22:04  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:11  peiyongz
 * sane_include
 *
 * Revision 1.6  2001/05/11 13:26:27  tng
 * Copyright update.
 *
 * Revision 1.5  2001/01/15 21:26:34  tng
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
 * Revision 1.4  2000/03/02 19:54:40  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.3  2000/02/24 20:05:24  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/06 07:48:02  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:04:37  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:45:08  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */

#if !defined(KVSTRINGPAIR_HPP)
#define KVSTRINGPAIR_HPP

#include <xercesc/util/XMemory.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/internal/XSerializable.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  This class provides a commonly used data structure, which is that of
//  a pair of strings which represent a 'key=value' type mapping. It works
//  only in terms of XMLCh type raw strings.
//
class XMLUTIL_EXPORT KVStringPair : public XSerializable, public XMemory
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    KVStringPair(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    KVStringPair
    (
        const XMLCh* const key
        , const XMLCh* const value
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    KVStringPair(const KVStringPair& toCopy);
    ~KVStringPair();


    // -----------------------------------------------------------------------
    //  Getters
    //
    //  We support the
    // -----------------------------------------------------------------------
    const XMLCh* getKey() const;
    XMLCh* getKey();
    const XMLCh* getValue() const;
    XMLCh* getValue();


    // -----------------------------------------------------------------------
    //  Setters
    // -----------------------------------------------------------------------
    void setKey(const XMLCh* const newKey);
    void setValue(const XMLCh* const newValue);
    void set
    (
        const   XMLCh* const    newKey
        , const XMLCh* const    newValue
    );


    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(KVStringPair)

private :
    // unimplemented:
       
    KVStringPair& operator=(const KVStringPair&);
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fKey
    //      The string that represents the key field of this object.
    //
    //  fKeyAllocSize
    //      The amount of memory allocated for fKey.
    //
    //  fValue
    //      The string that represents the value of this pair object.
    //
    //  fValueAllocSize
    //      The amount of memory allocated for fValue.
    //
    // -----------------------------------------------------------------------
    MemoryManager* fMemoryManager;
    XMLCh*         fKey;
    unsigned long  fKeyAllocSize;
    XMLCh*         fValue;
    unsigned long  fValueAllocSize;
};

// ---------------------------------------------------------------------------
//  KVStringPair: Getters
// ---------------------------------------------------------------------------
inline const XMLCh* KVStringPair::getKey() const
{
    return fKey;
}

inline XMLCh* KVStringPair::getKey()
{
    return fKey;
}

inline const XMLCh* KVStringPair::getValue() const
{
    return fValue;
}

inline XMLCh* KVStringPair::getValue()
{
    return fValue;
}

XERCES_CPP_NAMESPACE_END

#endif
