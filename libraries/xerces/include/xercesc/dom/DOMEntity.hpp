#ifndef DOMEntity_HEADER_GUARD_
#define DOMEntity_HEADER_GUARD_

/*
 * Copyright 2001-2002,2004 The Apache Software Foundation.
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
 * $Id: DOMEntity.hpp 176280 2005-01-07 15:32:34Z amassari $
 */


#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/dom/DOMNode.hpp>

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
    DOMEntity() {}
    DOMEntity(const DOMEntity &other) : DOMNode(other) {}
    //@}

private:
    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    /** @name Unimplemented operators */
    //@{
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

