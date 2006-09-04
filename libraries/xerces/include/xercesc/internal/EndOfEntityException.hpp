/*
 * Copyright 1999-2000,2004 The Apache Software Foundation.
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
 * $Id: EndOfEntityException.hpp 191054 2005-06-17 02:56:35Z jberry $
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
