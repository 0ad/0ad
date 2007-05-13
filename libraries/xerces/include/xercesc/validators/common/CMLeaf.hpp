/*
 * Copyright 1999-2001,2004 The Apache Software Foundation.
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
 * $Id: CMLeaf.hpp 191054 2005-06-17 02:56:35Z jberry $
 */

#if !defined(CMLEAF_HPP)
#define CMLEAF_HPP

#include <xercesc/validators/common/CMNode.hpp>


XERCES_CPP_NAMESPACE_BEGIN

//
//  This class represents a leaf in the content spec node tree of an
//  element's content model. It just has an element qname and a position value,
//  the latter of which is used during the building of a DFA.
//
class CMLeaf : public CMNode
{
public :
    // -----------------------------------------------------------------------
    //  Constructors
    // -----------------------------------------------------------------------
    CMLeaf
    (
          QName* const         element
        , const unsigned int   position = (~0)
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    CMLeaf
    (
          QName* const         element
        , const unsigned int   position
        , const bool           adopt
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ~CMLeaf();


    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    QName* getElement();
    const QName* getElement() const;
    unsigned int getPosition() const;


    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setPosition(const unsigned int newPosition);


    // -----------------------------------------------------------------------
    //  Implementation of public CMNode virtual interface
    // -----------------------------------------------------------------------
    bool isNullable() const;


protected :
    // -----------------------------------------------------------------------
    //  Implementation of protected CMNode virtual interface
    // -----------------------------------------------------------------------
    void calcFirstPos(CMStateSet& toSet) const;
    void calcLastPos(CMStateSet& toSet) const;


private :
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fElement
    //      This is the element that this leaf represents.
    //
    //  fPosition
    //      Part of the algorithm to convert a regex directly to a DFA
    //      numbers each leaf sequentially. If its -1, that means its an
    //      epsilon node. All others are non-epsilon positions.
    //
    //  fAdopt
    //      This node is responsible for the storage of the fElement QName.
    // -----------------------------------------------------------------------
    QName*          fElement;
    unsigned int    fPosition;
    bool            fAdopt;

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    CMLeaf(const CMLeaf&);
    CMLeaf& operator=(const CMLeaf&);
};


// -----------------------------------------------------------------------
//  Constructors
// -----------------------------------------------------------------------
inline CMLeaf::CMLeaf(       QName* const         element
                     , const unsigned int         position
                     ,       MemoryManager* const manager) :
    CMNode(ContentSpecNode::Leaf, manager)
    , fElement(0)
    , fPosition(position)
    , fAdopt(false)
{
    if (!element)
    {
        fElement = new (fMemoryManager) QName
        (
              XMLUni::fgZeroLenString
            , XMLUni::fgZeroLenString
            , XMLElementDecl::fgInvalidElemId
            , fMemoryManager
        );
        // We have to be responsible for this QName - override default fAdopt
        fAdopt = true;
    }
    else
    {
        fElement = element;
    }
}

inline CMLeaf::CMLeaf(       QName* const         element
                     , const unsigned int         position
                     , const bool                 adopt
                     ,       MemoryManager* const manager) :
    CMNode(ContentSpecNode::Leaf, manager)
    , fElement(0)
    , fPosition(position)
    , fAdopt(adopt)
{
    if (!element)
    {
        fElement = new (fMemoryManager) QName
        (
              XMLUni::fgZeroLenString
            , XMLUni::fgZeroLenString
            , XMLElementDecl::fgInvalidElemId
            , fMemoryManager
        );
        // We have to be responsible for this QName - override adopt parameter
        fAdopt = true;
    }
    else
    {
        fElement = element;
    }
}

inline CMLeaf::~CMLeaf()
{
    if (fAdopt)
        delete fElement;
}


// ---------------------------------------------------------------------------
//  Getter methods
// ---------------------------------------------------------------------------
inline QName* CMLeaf::getElement()
{
    return fElement;
}

inline const QName* CMLeaf::getElement() const
{
    return fElement;
}

inline unsigned int CMLeaf::getPosition() const
{
    return fPosition;
}


// ---------------------------------------------------------------------------
//  Setter methods
// ---------------------------------------------------------------------------
inline void CMLeaf::setPosition(const unsigned int newPosition)
{
    fPosition = newPosition;
}


// ---------------------------------------------------------------------------
//  Implementation of public CMNode virtual interface
// ---------------------------------------------------------------------------
inline bool CMLeaf::isNullable() const
{
    // Leaf nodes are never nullable unless its an epsilon node
    return (fPosition == -1);
}


// ---------------------------------------------------------------------------
//  Implementation of protected CMNode virtual interface
// ---------------------------------------------------------------------------
inline void CMLeaf::calcFirstPos(CMStateSet& toSet) const
{
    // If we are an epsilon node, then the first pos is an empty set
    if (fPosition == -1)
    {
        toSet.zeroBits();
        return;
    }

    // Otherwise, its just the one bit of our position
    toSet.setBit(fPosition);
}

inline void CMLeaf::calcLastPos(CMStateSet& toSet) const
{
    // If we are an epsilon node, then the last pos is an empty set
    if (fPosition == -1)
    {
        toSet.zeroBits();
        return;
    }

    // Otherwise, its just the one bit of our position
    toSet.setBit(fPosition);
}

XERCES_CPP_NAMESPACE_END

#endif
