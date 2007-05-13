/*
 * Copyright 2001,2004 The Apache Software Foundation.
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
 * $Id: ContentLeafNameTypeVector.hpp 191054 2005-06-17 02:56:35Z jberry $
 */


#if !defined(CONTENTLEAFNAMETYPEVECTOR_HPP)
#define CONTENTLEAFNAMETYPEVECTOR_HPP

#include <xercesc/validators/common/ContentSpecNode.hpp>
#include <xercesc/framework/MemoryManager.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLPARSER_EXPORT ContentLeafNameTypeVector : public XMemory
{
public :
    // -----------------------------------------------------------------------
    //  Class specific types
    // -----------------------------------------------------------------------


    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    ContentLeafNameTypeVector
    (
        MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    ContentLeafNameTypeVector
    (
        QName** const                     qName
      , ContentSpecNode::NodeTypes* const types
      , const unsigned int                count
      , MemoryManager* const              manager = XMLPlatformUtils::fgMemoryManager
    );

    ~ContentLeafNameTypeVector();

    ContentLeafNameTypeVector(const ContentLeafNameTypeVector&);

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    QName* getLeafNameAt(const unsigned int pos) const;

    const ContentSpecNode::NodeTypes getLeafTypeAt(const unsigned int pos) const;
    const unsigned int getLeafCount() const;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void setValues
    (
        QName** const                      qName
      , ContentSpecNode::NodeTypes* const  types
      , const unsigned int                       count
    );

    // -----------------------------------------------------------------------
    //  Miscellaneous
    // -----------------------------------------------------------------------

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ContentLeafNameTypeVector& operator=(const ContentLeafNameTypeVector&);

    // -----------------------------------------------------------------------
    //  helper methods
    // -----------------------------------------------------------------------
    void cleanUp();
    void init(const unsigned int);

    // -----------------------------------------------------------------------
    //  Private Data Members
    //
    // -----------------------------------------------------------------------
    MemoryManager*                fMemoryManager;
    QName**                       fLeafNames;
    ContentSpecNode::NodeTypes   *fLeafTypes;
    unsigned int                  fLeafCount;
};

inline void ContentLeafNameTypeVector::cleanUp()
{
	fMemoryManager->deallocate(fLeafNames); //delete [] fLeafNames;
	fMemoryManager->deallocate(fLeafTypes); //delete [] fLeafTypes;
}

inline void ContentLeafNameTypeVector::init(const unsigned int size)
{
    fLeafNames = (QName**) fMemoryManager->allocate(size * sizeof(QName*));//new QName*[size];
    fLeafTypes = (ContentSpecNode::NodeTypes *) fMemoryManager->allocate
    (
        size * sizeof(ContentSpecNode::NodeTypes)
    ); //new ContentSpecNode::NodeTypes [size];
    fLeafCount = size;
}

XERCES_CPP_NAMESPACE_END

#endif
