#ifndef DOMCDataSection_HEADER_GUARD_
#define DOMCDataSection_HEADER_GUARD_


/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2001-2002 The Apache Software Foundation.  All rights
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
 * $Id: DOMCDATASection.hpp,v 1.6 2003/03/07 19:58:58 tng Exp $
 */

#include <xercesc/util/XercesDefs.hpp>
#include "DOMText.hpp"

XERCES_CPP_NAMESPACE_BEGIN


/**
 * CDATA sections are used to escape blocks of text containing characters that
 * would otherwise be regarded as markup. The only delimiter that is
 * recognized in a CDATA section is the "]]&gt;" string that ends the CDATA
 * section. CDATA sections cannot be nested. Their primary purpose is for
 * including material such as XML fragments, without needing to escape all
 * the delimiters.
 * <p>The <code>data</code> attribute of the <code>DOMText</code> node holds
 * the text that is contained by the CDATA section. Note that this may
 * contain characters that need to be escaped outside of CDATA sections and
 * that, depending on the character encoding ("charset") chosen for
 * serialization, it may be impossible to write out some characters as part
 * of a CDATA section.
 * <p>The <code>DOMCDATASection</code> interface inherits from the
 * <code>DOMCharacterData</code> interface through the <code>DOMText</code>
 * interface. Adjacent <code>DOMCDATASection</code> nodes are not merged by use
 * of the <code>normalize</code> method of the <code>DOMNode</code> interface.
 * Because no markup is recognized within a <code>DOMCDATASection</code>,
 * character numeric references cannot be used as an escape mechanism when
 * serializing. Therefore, action needs to be taken when serializing a
 * <code>DOMCDATASection</code> with a character encoding where some of the
 * contained characters cannot be represented. Failure to do so would not
 * produce well-formed XML.One potential solution in the serialization
 * process is to end the CDATA section before the character, output the
 * character using a character reference or entity reference, and open a new
 * CDATA section for any further characters in the text node. Note, however,
 * that some code conversion libraries at the time of writing do not return
 * an error or exception when a character is missing from the encoding,
 * making the task of ensuring that data is not corrupted on serialization
 * more difficult.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a>.
 *
 * @since DOM Level 1
 */
class CDOM_EXPORT DOMCDATASection: public DOMText {
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMCDATASection() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMCDATASection(const DOMCDATASection &);
    DOMCDATASection & operator = (const DOMCDATASection &);
    //@}

public:
    // -----------------------------------------------------------------------
    //  All constructors are hidden, just the destructor is available
    // -----------------------------------------------------------------------
    /** @name Destructor */
    //@{
    /**
     * Destructor
     *
     */
    virtual ~DOMCDATASection() {};
    //@}

};

XERCES_CPP_NAMESPACE_END

#endif


