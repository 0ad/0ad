#ifndef DOMEntity_HEADER_GUARD_
#define DOMEntity_HEADER_GUARD_

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
 * $Id: DOMEntity.hpp,v 1.7 2003/03/07 19:59:03 tng Exp $
 */


#include <xercesc/util/XercesDefs.hpp>
#include "DOMNode.hpp"

XERCES_CPP_NAMESPACE_BEGIN

/**
 * This interface represents an entity, either parsed or unparsed, in an XML
 * document. Note that this models the entity itself not the entity
 * declaration. <code>DOMEntity</code> declaration modeling has been left for a
 * later Level of the DOM specification.
 * <p>The <code>nodeName</code> attribute that is inherited from
 * <code>DOMNode</code> contains the name of the entity.
 * <p>An XML processor may choose to completely expand entities before the
 * structure model is passed to the DOM; in this case there will be no
 * <code>DOMEntityReference</code> nodes in the document tree.
 * <p>XML does not mandate that a non-validating XML processor read and
 * process entity declarations made in the external subset or declared in
 * external parameter entities. This means that parsed entities declared in
 * the external subset need not be expanded by some classes of applications,
 * and that the replacement value of the entity may not be available. When
 * the replacement value is available, the corresponding <code>DOMEntity</code>
 * node's child list represents the structure of that replacement text.
 * Otherwise, the child list is empty.
 * <p>The DOM Level 2 does not support editing <code>DOMEntity</code> nodes; if a
 * user wants to make changes to the contents of an <code>DOMEntity</code>,
 * every related <code>DOMEntityReference</code> node has to be replaced in the
 * structure model by a clone of the <code>DOMEntity</code>'s contents, and
 * then the desired changes must be made to each of those clones instead.
 * <code>DOMEntity</code> nodes and all their descendants are readonly.
 * <p>An <code>DOMEntity</code> node does not have any parent.If the entity
 * contains an unbound namespace prefix, the <code>namespaceURI</code> of
 * the corresponding node in the <code>DOMEntity</code> node subtree is
 * <code>null</code>. The same is true for <code>DOMEntityReference</code>
 * nodes that refer to this entity, when they are created using the
 * <code>createEntityReference</code> method of the <code>DOMDocument</code>
 * interface. The DOM Level 2 does not support any mechanism to resolve
 * namespace prefixes.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a>.
 *
 * @since DOM Level 1
 */
class CDOM_EXPORT DOMEntity: public DOMNode {
protected:
    // -----------------------------------------------------------------------
    //  Hidden constructors
    // -----------------------------------------------------------------------
    /** @name Hidden constructors */
    //@{    
    DOMEntity() {};
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented constructors and operators */
    //@{
    DOMEntity(const DOMEntity &);
    DOMEntity & operator = (const DOMEntity &);
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
    virtual ~DOMEntity() {};
    //@}

    // -----------------------------------------------------------------------
    //  Virtual DOMEntity interface
    // -----------------------------------------------------------------------
    /** @name Functions introduced in DOM Level 1 */
    //@{
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    /**
     * The public identifier associated with the entity, if specified.
     *
     * If the public identifier was not specified, this is <code>null</code>.
     *
     * @since DOM Level 1
     */
    virtual const XMLCh *        getPublicId() const = 0;

    /**
     * The system identifier associated with the entity, if specified.
     *
     * If the system identifier was not specified, this is <code>null</code>.
     *
     * @since DOM Level 1
     */
    virtual const XMLCh *        getSystemId() const = 0;

    /**
     * For unparsed entities, the name of the notation for the entity.
     *
     * For parsed entities, this is <code>null</code>.
     *
     * @since DOM Level 1
     */
    virtual const XMLCh *        getNotationName() const = 0;
    //@}

    /** @name Functions introduced in DOM Level 3. */
    //@{

     /**
     * An attribute specifying the actual encoding of this entity, when it is
     * an external parsed entity. This is <code>null</code> otherwise.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @since DOM Level 3
     */
    virtual const XMLCh*           getActualEncoding() const = 0;

    /**
     * An attribute specifying the actual encoding of this entity, when it is
     * an external parsed entity. This is <code>null</code> otherwise.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @since DOM Level 3
     */
    virtual void                   setActualEncoding(const XMLCh* actualEncoding) = 0;

    /**
     * An attribute specifying, as part of the text declaration, the encoding
     * of this entity, when it is an external parsed entity. This is
     * <code>null</code> otherwise.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @since DOM Level 3
     */
    virtual const XMLCh*           getEncoding() const = 0;

    /**
     * An attribute specifying, as part of the text declaration, the encoding
     * of this entity, when it is an external parsed entity. This is
     * <code>null</code> otherwise.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @since DOM Level 3
     */
    virtual void                   setEncoding(const XMLCh* encoding) = 0;

    /**
     * An attribute specifying, as part of the text declaration, the version
     * number of this entity, when it is an external parsed entity. This is
     * <code>null</code> otherwise.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @since DOM Level 3
     */
    virtual const XMLCh*           getVersion() const = 0;

    /**
     * An attribute specifying, as part of the text declaration, the version
     * number of this entity, when it is an external parsed entity. This is
     * <code>null</code> otherwise.
     *
     * <p><b>"Experimental - subject to change"</b></p>
     *
     * @since DOM Level 3
     */
    virtual void                   setVersion(const XMLCh* version) = 0;
    //@}
};

XERCES_CPP_NAMESPACE_END

#endif

