/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2002 The Apache Software Foundation.  All rights
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
 * $Id: DOM_Range.hpp,v 1.3 2002/11/04 15:04:44 tng Exp $
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

class CDOM_EXPORT DOM_Range {
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
