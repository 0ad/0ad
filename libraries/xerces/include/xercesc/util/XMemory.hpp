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
 * $Id: XMemory.hpp,v 1.6 2004/01/15 19:48:23 knoaman Exp $
 */


#if !defined(XMEMORY_HPP)
#define XMEMORY_HPP

#include <xercesc/util/XercesDefs.hpp>
#include <stdlib.h>

XERCES_CPP_NAMESPACE_BEGIN

class MemoryManager;

/**
 *  This class makes it possible to override the C++ memory management by
 *  adding new/delete operators to this base class.
 *
 *  This class is used in conjuction with the pluggable memory manager. It
 *  allows applications to control Xerces memory management.
 */

class XMLUTIL_EXPORT XMemory
{
public :
    // -----------------------------------------------------------------------
    //  The C++ memory management
    // -----------------------------------------------------------------------
    /** @name The C++ memory management */
    //@{

    /**
      * This method overrides operator new
      *
      * @param size The requested memory size
      */
    void* operator new(size_t size);

#if defined(XML_VISUALCPP)
    /**
      * This method overrides the MFC debug version of the operator new
      * 
      * @param size   The requested memory size
      * @param file   The file where the allocation was requested
      * @param line   The line where the allocation was requested 
      */ 
    void* operator new(size_t size, const char* file, int line);
    /**
      * This method provides a matching delete for the MFC debug new
      * 
      * @param p      The pointer to the allocated memory
      * @param file   The file where the allocation was requested
      * @param line   The line where the allocation was requested 
      */ 
    void operator delete(void* p, const char* file, int line);
#endif

    /**
      * This method overrides placement operator new
      *
      * @param size   The requested memory size
      * @param memMgr An application's memory manager
      */
    void* operator new(size_t size, MemoryManager* memMgr);

    /**
      * This method overrides operator delete
      *
      * @param p The pointer to the allocated memory
      */
    void operator delete(void* p);

     //The Borland compiler is complaining about duplicate overloading of delete
#if !defined(XML_BORLAND)
    /**
      * This method provides a matching delete for the placement new
      *
      * @param p      The pointer to the allocated memory
      * @param memMgr An application's memory manager 
      */
    void operator delete(void* p, MemoryManager* memMgr);
#endif

    //@}

protected :
    // -----------------------------------------------------------------------
    //  Hidden Constructors
    // -----------------------------------------------------------------------
    /** @name Constructor */
    //@{

    /**
      * Protected default constructor and copy constructor
      */
    XMemory()
    {
    }

    XMemory(const XMemory&)
    {
    }
    //@}

#if defined(XML_BORLAND)
    virtual ~XMemory()
    {
    }
#endif

private:
    // -----------------------------------------------------------------------
    //  Unimplemented operators
    // -----------------------------------------------------------------------
    XMemory& operator=(const XMemory&);
};

XERCES_CPP_NAMESPACE_END

#endif
