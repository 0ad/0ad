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
 * $Id: DOM_Range.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#ifndef DOM_Range_HEADER_GUARD_
#define DOM_Range_HEADER_GUARD_

#include <xercesc/util/XercesDefs.hpp>
#include "DOM_Node.hpp"
#include "DOMString.hpp"
#include "DOM_DocumentFragment.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class RangeImpl;

//class RangeImpl;

class DEPRECATED_DOM_EXPORT DOM_Range {
public:

    enum CompareHow {
        START_TO_START  = 0,
        START_TO_END    = 1,
        END_TO_END      = 2,
        END_TO_START    = 3
    };

    //c'tor & d'tor
    DOM_Range();
    DOM_Range(const DOM_Range& other);
    ~DOM_Range();


    DOM_Range & operator = (const DOM_Range &other);
    DOM_Range & operator = (const DOM_NullPtr *other);
    bool operator != (const DOM_Range & other) const;
    bool operator == (const DOM_Range & other) const;
    bool operator != (const DOM_NullPtr * other) const;
    bool operator == (const DOM_NullPtr * other) const;

    //getter functions
    DOM_Node getStartContainer() const;
    unsigned int getStartOffset() const;
    DOM_Node getEndContainer() const;
    unsigned int getEndOffset() const;
    bool getCollapsed() const;
    const DOM_Node getCommonAncestorContainer() const;

    //setter functions
    void setStart(const DOM_Node &parent, unsigned int offset);
    void setEnd(const DOM_Node &parent, unsigned int offset);

    void setStartBefore(const DOM_Node &refNode);
    void setStartAfter(const DOM_Node &refNode);
    void setEndBefore(const DOM_Node &refNode);
    void setEndAfter(const DOM_Node &refNode);

    //misc functions
    void collapse(bool toStart);
    void selectNode(const DOM_Node &node);
    void selectNodeContents(const DOM_Node &node);

    //Functions related to comparing range Boundrary-Points
    short compareBoundaryPoints(CompareHow how, const DOM_Range& range) const;
    void deleteContents();
    DOM_DocumentFragment extractContents();
    DOM_DocumentFragment cloneContents() const;
    void insertNode(DOM_Node& node);
    //Misc functions
    void surroundContents(DOM_Node &node);
    DOM_Range cloneRange() const;
    DOMString toString() const;
    void detach();




protected:

    DOM_Range(RangeImpl *);
    RangeImpl   *fImpl;

    friend class DOM_Document;
};




XERCES_CPP_NAMESPACE_END

#endif
