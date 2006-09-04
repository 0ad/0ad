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
 * $Id: DOM_NodeFilter.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

// DOM_NodeFilter.h: interface for the DOM_NodeFilter class.
//
//////////////////////////////////////////////////////////////////////

#ifndef DOM_NodeFilter_HEADER_GUARD_
#define DOM_NodeFilter_HEADER_GUARD_

#include "DOM_Node.hpp"

XERCES_CPP_NAMESPACE_BEGIN


class NodeFilterImpl;


/**
 * Filters are objects that know how to "filter out" nodes. If a
 * <code>DOM_NodeIterator</code> or <code>DOM_TreeWalker</code> is given a
 * filter, it applies the filter before it returns the next node.
 *
 * If the filter says to accept the node, the iterator returns it; otherwise, the
 * iterator looks for the next node and pretends that the node that was rejected
 * was not there.
 *
 *  The DOM does not provide any filters. Filter is just an interface that users can
 *  implement to provide their own filters.
 *
 *  Filters do not need to know how to iterate, nor do they need to know anything
 *  about the data structure that is being iterated. This makes it very easy to write
 *  filters, since the only thing they have to know how to do is evaluate a single node.
 *  One filter may be used with a number of different kinds of iterators, encouraging
 *  code reuse.
 *
 */
class DEPRECATED_DOM_EXPORT DOM_NodeFilter
{
    public:
	/** @name Enumerators for Node Filter */
        //@{
	/*
	  *		<table><tr><td>FILTER_ACCEPT</td>
      *            <td>Accept the node. Navigation methods defined for
      *                NodeIterator or TreeWalker will return this node.</td>
	  *			</tr>
	  *			<tr><td>
      *               FILTER_REJECT</td>
      *               <td>Reject the node. Navigation methods defined for
      *               NodeIterator or TreeWalker will not return this
      *               node. For TreeWalker, the children of this node will
      *               also be rejected. Iterators treat this as a synonym
      *               for FILTER_SKIP.</td>
	  *			</tr>
	  *			<tr><td>FILTER_SKIP</td>
      *              <td>Reject the node. Navigation methods defined for
      *                  NodeIterator or TreeWalker will not return this
      *                  node. For both NodeIterator and Treewalker, the
      *                  children of this node will still be considered.</td>
 	  *			</tr>
	  *		</table>
      *
	  */
        enum FilterAction {FILTER_ACCEPT = 1,
                           FILTER_REJECT = 2,
                           FILTER_SKIP   = 3};

        enum ShowType {
            SHOW_ALL                       = 0x0000FFFF,
            SHOW_ELEMENT                   = 0x00000001,
            SHOW_ATTRIBUTE                 = 0x00000002,
            SHOW_TEXT                      = 0x00000004,
            SHOW_CDATA_SECTION             = 0x00000008,
            SHOW_ENTITY_REFERENCE          = 0x00000010,
            SHOW_ENTITY                    = 0x00000020,
            SHOW_PROCESSING_INSTRUCTION    = 0x00000040,
            SHOW_COMMENT                   = 0x00000080,
            SHOW_DOCUMENT                  = 0x00000100,
            SHOW_DOCUMENT_TYPE             = 0x00000200,
            SHOW_DOCUMENT_FRAGMENT         = 0x00000400,
            SHOW_NOTATION                  = 0x00000800
        };
	//@}

        /** @name Constructors */
        //@{
        /**
          * Default constructor for DOM_NodeFilter.
          */
        DOM_NodeFilter();
        //@}

        /** @name Destructor. */
        //@{
	/**
	  * Destructor for DOM_NodeFilter.
	  */
        virtual ~DOM_NodeFilter();
        //@}

        /** @name Test function. */
        //@{
	/**
	  * Test whether a specified node is visible in the logical view of a DOM_TreeWalker
	  * or DOM_NodeIterator. This function will be called by the implementation of
	  * DOM_TreeWalker and DOM_NodeIterator; it is not intended to be called directly from user
	  * code.
          *
          * @param node The node to check to see if it passes the filter or not.
          * @return A constant to determine whether the node is accepted, rejected, or skipped.
	  */
        virtual short acceptNode (const DOM_Node &node) const =0;
        //@}

    private:
        DOM_NodeFilter(const DOM_NodeFilter &other);
        DOM_NodeFilter & operator = (const DOM_NodeFilter &other);
        bool operator == (const DOM_NodeFilter &other) const;
        bool operator != (const DOM_NodeFilter &other) const;
};

XERCES_CPP_NAMESPACE_END

#endif
