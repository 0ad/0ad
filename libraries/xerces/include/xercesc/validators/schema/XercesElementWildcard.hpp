/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001 The Apache Software Foundation.  All rights
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
 * originally based on software copyright (c) 2001, International
 * Business Machines, Inc., http://www.ibm.com .  For more information
 * on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

/*
 * $Log: XercesElementWildcard.hpp,v $
 * Revision 1.2  2002/11/04 14:49:42  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:50  peiyongz
 * sane_include
 *
 * Revision 1.2  2001/11/21 14:30:13  knoaman
 * Fix for UPA checking.
 *
 * Revision 1.1  2001/08/21 15:58:42  tng
 * Schema: New files XercesElementWildCard.
 *

*/


#if !defined(XERCESELEMENTWILDCARD_HPP)
#define XERCESELEMENTWILDCARD_HPP

#include <xercesc/util/QName.hpp>
#include <xercesc/validators/common/ContentSpecNode.hpp>
#include <xercesc/validators/schema/SubstitutionGroupComparator.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// ---------------------------------------------------------------------------
//  Forward declarations
// ---------------------------------------------------------------------------
class SchemaGrammar;


class VALIDATORS_EXPORT XercesElementWildcard
{

public :

    // -----------------------------------------------------------------------
    //  Class static methods
    // -----------------------------------------------------------------------
    /*
     * check whether two elements are in conflict
     */
    static bool conflict(SchemaGrammar* const         pGrammar,
                         ContentSpecNode::NodeTypes   type1,
                         QName*                       q1,
                         ContentSpecNode::NodeTypes   type2,
                         QName*                       q2,
                         SubstitutionGroupComparator* comparator);

private:

    // -----------------------------------------------------------------------
    //  private helper methods
    // -----------------------------------------------------------------------
    static bool uriInWildcard(SchemaGrammar* const         pGrammar,
                              QName*                       qname,
                              unsigned int                 wildcard,
                              ContentSpecNode::NodeTypes   wtype,
                              SubstitutionGroupComparator* comparator);

    static bool wildcardIntersect(ContentSpecNode::NodeTypes t1,
                                  unsigned int               w1,
                                  ContentSpecNode::NodeTypes t2,
                                  unsigned int               w2);

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XercesElementWildcard();
    ~XercesElementWildcard();
};

XERCES_CPP_NAMESPACE_END

#endif // XERCESELEMENTWILDCARD_HPP

