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
 * $Log: EndOfEntityException.hpp,v $
 * Revision 1.3  2004/01/29 11:46:30  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.2  2002/11/04 14:58:18  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:21:58  peiyongz
 * sane_include
 *
 * Revision 1.3  2000/02/24 20:18:07  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.2  2000/02/06 07:47:53  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:08:07  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:44:42  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(ENDOFENTITYEXCEPTION_HPP)
#define ENDOFENTITYEXCEPTION_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLEntityDecl;

//
//  This class is only used internally. Its thrown by the ReaderMgr class,
//  when an entity ends, and is caught in the scanner. This tells the scanner
//  that an entity has ended, and allows it to do the right thing according
//  to what was going on when the entity ended.
//
//  Since its internal, it does not bother implementing XMLException.
//
class XMLPARSER_EXPORT EndOfEntityException
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    EndOfEntityException(       XMLEntityDecl*  entityThatEnded
                        , const unsigned int    readerNum) :

        fEntity(entityThatEnded)
        , fReaderNum(readerNum)
    {
    }

    EndOfEntityException(const EndOfEntityException& toCopy) :

        fEntity(toCopy.fEntity)
        , fReaderNum(toCopy.fReaderNum)
    {
    }

    ~EndOfEntityException()
    {
    }


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    XMLEntityDecl& getEntity();
    const XMLEntityDecl& getEntity() const;
    unsigned int getReaderNum() const;


private :
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    EndOfEntityException& operator = (const  EndOfEntityException&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fEntity
    //      This is a reference to the entity that ended, causing this
    //      exception.
    //
    //  fReaderNum
    //      The unique reader number of the reader that was handling this
    //      entity. This is used to know whether a particular entity has
    //      ended.
    // -----------------------------------------------------------------------
    XMLEntityDecl*  fEntity;
    unsigned int    fReaderNum;
};


// ---------------------------------------------------------------------------
//  EndOfEntityException: Getter methods
// ---------------------------------------------------------------------------
inline XMLEntityDecl& EndOfEntityException::getEntity()
{
    return *fEntity;
}

inline const XMLEntityDecl& EndOfEntityException::getEntity() const
{
    return *fEntity;
}

inline unsigned int EndOfEntityException::getReaderNum() const
{
    return fReaderNum;
}

XERCES_CPP_NAMESPACE_END

#endif
