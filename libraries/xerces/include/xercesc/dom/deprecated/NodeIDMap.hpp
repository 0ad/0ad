#ifndef NodeIDMap_HEADER_GUARD_
#define NodeIDMap_HEADER_GUARD_

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


//
//  This file is part of the internal implementation of the C++ XML DOM.
//  It should NOT be included or used directly by application programs.
//
//  Applications should include the file <xercesc/dom/deprecated/DOM.hpp> for the entire
//  DOM API, or DOM_*.hpp for individual DOM classes, where the class
//  name is substituded for the *.
//




//
//  Class NodeIDMap is a hash table that is used in the implementation of
//   of DOM_Document::getElementsByID().
//
//  Why Yet Another HashTable implementation?  Becuase it can be significantly
//  smaller when tuned for this exact usage, and the generic RefHashTableOf
//  from the xerces utils project is not a paricularly good fit.
//
#include <xercesc/util/PlatformUtils.hpp>

XERCES_CPP_NAMESPACE_BEGIN


class AttrImpl;
class DOMString;


class NodeIDMap : public XMemory {
public:

    // Create a new hash table, sized to hold "initialSize"
    NodeIDMap(int initialSize,
              MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
                                   //  Entries.  It will automatically grow if need be.

    virtual ~NodeIDMap();

private:
    NodeIDMap(const NodeIDMap &other);   // No copy, assignement, comparison.
    NodeIDMap &operator = (const NodeIDMap &other);
    bool operator == (const NodeIDMap &other);

public:
    void  add(AttrImpl *attr);       // Add the specified attribute to the table.
    void  remove(AttrImpl *other);   // Remove the specified attribute.
                                           //   Does nothing if the node is not in the table.
    AttrImpl *find(const DOMString &ID);   // Find the attribute node in the table with this ID

private:
    void growTable();

private:
    AttrImpl      **fTable;
    unsigned int  fSizeIndex;              // Index of the current table size in the
                                           //   array of possible table sizes.
	unsigned int  fSize;                   // The current size of the table array
                                           //   (number of slots, not bytes.)
    unsigned int  fNumEntries;             // The number of entries used.
    unsigned int  fMaxEntries;             // The max number of entries to use before
                                           //   growing the table.
    MemoryManager* fMemoryManager;

};

XERCES_CPP_NAMESPACE_END

#endif
