/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
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
 * $Id: XMLNetAccessor.hpp,v 1.5 2003/05/15 19:07:46 knoaman Exp $
 */

#if !defined(XMLNETACCESSOR_HPP)
#define XMLNETACCESSOR_HPP

#include <xercesc/util/XMLURL.hpp>
#include <xercesc/util/XMLException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class BinInputStream;



//
//  This class is an abstract interface via which the URL class accesses
//  net access services. When any source URL is not in effect a local file
//  path, then the URL class is used to look at it. Then the URL class can
//  be asked to make a binary input stream via which the referenced resource
//  can be read in.
//
//  The URL class will use an object derived from this class to create a
//  binary stream for the URL to return. The object it uses is provided by
//  the platform utils, and is actually provided by the per-platform init
//  code so each platform can decide what actual implementation it wants to
//  use.
//
class XMLUTIL_EXPORT XMLNetAccessor : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Virtual destructor
    // -----------------------------------------------------------------------
    virtual ~XMLNetAccessor()
    {
    }


    // -----------------------------------------------------------------------
    //  The virtual net accessor interface
    // -----------------------------------------------------------------------
    virtual const XMLCh* getId() const = 0;

    virtual BinInputStream* makeNew
    (
        const   XMLURL&                 urlSrc
    ) = 0;


protected :
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    XMLNetAccessor()
    {
    }


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLNetAccessor(const XMLNetAccessor&);
    XMLNetAccessor& operator=(const XMLNetAccessor&);
};

MakeXMLException(NetAccessorException, XMLUTIL_EXPORT)

XERCES_CPP_NAMESPACE_END

#endif
