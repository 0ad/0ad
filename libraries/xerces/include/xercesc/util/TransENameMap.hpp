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


#if !defined(TRANSENAMEMAP_HPP)
#define TRANSENAMEMAP_HPP

#include <xercesc/util/TransService.hpp>
#include <xercesc/util/XMLString.hpp>

XERCES_CPP_NAMESPACE_BEGIN

//
//  This class is really private to the TransService class. However, some
//  compilers are too dumb to allow us to hide this class there in the Cpp
//  file that uses it.
//
class ENameMap : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Destructor
    // -----------------------------------------------------------------------
    virtual ~ENameMap()
    {
        //delete [] fEncodingName;
        XMLPlatformUtils::fgMemoryManager->deallocate(fEncodingName);
    }



    // -----------------------------------------------------------------------
    //  Virtual factory method
    // -----------------------------------------------------------------------
    virtual XMLTranscoder* makeNew
    (
        const   unsigned int    blockSize
        , MemoryManager*  const manager = XMLPlatformUtils::fgMemoryManager
    )   const = 0;


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    const XMLCh* getKey() const
    {
        return fEncodingName;
    }


protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    ENameMap(const XMLCh* const encodingName) :
          fEncodingName(XMLString::replicate(encodingName, XMLPlatformUtils::fgMemoryManager))
    {
    }


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ENameMap();
    ENameMap(const ENameMap&);
    ENameMap& operator=(const ENameMap&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fEncodingName
    //      This is the encoding name for the transcoder that is controlled
    //      by this map instance.
    // -----------------------------------------------------------------------
    XMLCh*  fEncodingName;
};


template <class TType> class ENameMapFor : public ENameMap
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ENameMapFor(const XMLCh* const encodingName);
    ~ENameMapFor();


    // -----------------------------------------------------------------------
    //  Implementation of virtual factory method
    // -----------------------------------------------------------------------
    virtual XMLTranscoder* makeNew(const unsigned int blockSize,
                                   MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) const;


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ENameMapFor();
    ENameMapFor(const ENameMapFor<TType>&);
    ENameMapFor<TType>& operator=(const ENameMapFor<TType>&);
};


template <class TType> class EEndianNameMapFor : public ENameMap
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    EEndianNameMapFor(const XMLCh* const encodingName, const bool swapped);
    ~EEndianNameMapFor();


    // -----------------------------------------------------------------------
    //  Implementation of virtual factory method
    // -----------------------------------------------------------------------
    virtual XMLTranscoder* makeNew(const unsigned int blockSize,
                                   MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager) const;


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    EEndianNameMapFor(const EEndianNameMapFor<TType>&);
    EEndianNameMapFor<TType>& operator=(const EEndianNameMapFor<TType>&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fSwapped
    //      Indicates whether the endianess of the encoding is opposite of
    //      that of the local host.
    // -----------------------------------------------------------------------
    bool    fSwapped;
};

XERCES_CPP_NAMESPACE_END

#if !defined(XERCES_TMPLSINC)
#include <xercesc/util/TransENameMap.c>
#endif

#endif
