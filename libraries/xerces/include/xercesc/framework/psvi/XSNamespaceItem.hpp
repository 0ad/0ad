/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2003 The Apache Software Foundation.  All rights
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
 * $Log: XSNamespaceItem.hpp,v $
 * Revision 1.8  2003/12/24 17:42:02  knoaman
 * Misc. PSVI updates
 *
 * Revision 1.7  2003/12/01 23:23:26  neilg
 * fix for bug 25118; thanks to Jeroen Witmond
 *
 * Revision 1.6  2003/11/21 22:34:45  neilg
 * More schema component model implementation, thanks to David Cargill.
 * In particular, this cleans up and completes the XSModel, XSNamespaceItem,
 * XSAttributeDeclaration and XSAttributeGroup implementations.
 *
 * Revision 1.5  2003/11/21 17:34:04  knoaman
 * PSVI update
 *
 * Revision 1.4  2003/11/14 22:47:53  neilg
 * fix bogus log message from previous commit...
 *
 * Revision 1.3  2003/11/14 22:33:30  neilg
 * Second phase of schema component model implementation.  
 * Implement XSModel, XSNamespaceItem, and the plumbing necessary
 * to connect them to the other components.
 * Thanks to David Cargill.
 *
 * Revision 1.2  2003/11/06 15:30:04  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.1  2003/09/16 14:33:36  neilg
 * PSVI/schema component model classes, with Makefile/configuration changes necessary to build them
 *
 */

#if !defined(XSNAMESPACEITEM_HPP)
#define XSNAMESPACEITEM_HPP

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/psvi/XSObject.hpp>
#include <xercesc/framework/psvi/XSNamedMap.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * This class contains all properties of the Schema Namespace Information infoitem.  
 * These items correspond to the result of processing a schema document
 * and all its included/redefined schema documents.  It corresponds to the
 * schema component discussed in the schema specifications, but since it
 * is not like other components does not inherit from the XSObject interface.
 * This is *always* owned by the validator /parser object from which
 * it is obtained.  It is designed to be subclassed; subclasses will
 * specify under what conditions it may be relied upon to have meaningful contents.
 */

// forward declarations
class XSAnnotation;
class XSAttributeDeclaration;
class XSAttributeGroupDefinition;
class XSElementDeclaration;
class XSModelGroupDefinition;
class XSNotationDeclaration;
class XSTypeDefinition;
class SchemaGrammar;
class XSModel;

class XMLPARSER_EXPORT XSNamespaceItem : public XMemory
{
public:

    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    /**
      * The default constructor 
      *
      * @param  xsModel
      * @param  grammar
      * @param  manager     The configurable memory manager
      */
    XSNamespaceItem
    (
        XSModel* const         xsModel
        , SchemaGrammar* const grammar
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    XSNamespaceItem
    (
        XSModel* const         xsModel
        , const XMLCh* const   schemaNamespace
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    //@};

    /** @name Destructor */
    //@{
    ~XSNamespaceItem();
    //@}

    //---------------------
    /** @name XSNamespaceItem methods */

    //@{

    /**
     * [schema namespace]: A namespace name or <code>null</code>
     * corresponding to the target namespace of the schema document.
     */
    const XMLCh *getSchemaNamespace();

    /**
     * [schema components]: a list of top-level components, i.e. element 
     * declarations, attribute declarations, etc. 
     * @param objectType The type of the declaration, i.e. 
     *   <code>ELEMENT_DECLARATION</code>, 
     *   <code>TYPE_DEFINITION</code> and any other component type that
     * may be a property of a schema component.
     * @return A list of top-level definition of the specified type in 
     *   <code>objectType</code> or <code>null</code>. 
     */
    XSNamedMap<XSObject> *getComponents(XSConstants::COMPONENT_TYPE objectType);

    /**
     *  [annotations]: a set of annotations.
     */
    XSAnnotationList *getAnnotations();

    /**
     * Convenience method. Returns a top-level element declaration. 
     * @param name The name of the declaration.
     * @return A top-level element declaration or <code>null</code> if such 
     *   declaration does not exist. 
     */
    XSElementDeclaration *getElementDeclaration(const XMLCh *name);

    /**
     * Convenience method. Returns a top-level attribute declaration. 
     * @param name The name of the declaration.
     * @return A top-level attribute declaration or <code>null</code> if such 
     *   declaration does not exist. 
     */
    XSAttributeDeclaration *getAttributeDeclaration(const XMLCh *name);

    /**
     * Convenience method. Returns a top-level simple or complex type 
     * definition. 
     * @param name The name of the definition.
     * @return An <code>XSTypeDefinition</code> or <code>null</code> if such 
     *   definition does not exist. 
     */
    XSTypeDefinition *getTypeDefinition(const XMLCh *name);

    /**
     * Convenience method. Returns a top-level attribute group definition. 
     * @param name The name of the definition.
     * @return A top-level attribute group definition or <code>null</code> if 
     *   such definition does not exist. 
     */
    XSAttributeGroupDefinition *getAttributeGroup(const XMLCh *name);

    /**
     * Convenience method. Returns a top-level model group definition. 
     * @param name The name of the definition.
     * @return A top-level model group definition definition or 
     *   <code>null</code> if such definition does not exist. 
     */
    XSModelGroupDefinition *getModelGroupDefinition(const XMLCh *name);

    /**
     * Convenience method. Returns a top-level notation declaration. 
     * @param name The name of the declaration.
     * @return A top-level notation declaration or <code>null</code> if such 
     *   declaration does not exist. 
     */
    XSNotationDeclaration *getNotationDeclaration(const XMLCh *name);

    /**
     * [document location] - a list of locations URI for the documents that 
     * contributed to the XSModel.
     */
    StringList *getDocumentLocations();

    //@}

    //----------------------------------
    /** methods needed by implementation */

    //@{


    //@}
private:

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XSNamespaceItem(const XSNamespaceItem&);
    XSNamespaceItem & operator=(const XSNamespaceItem &);

protected:
    friend class XSModel;
    friend class XSObjectFactory;
    // -----------------------------------------------------------------------
    //  data members
    // -----------------------------------------------------------------------
    // fMemoryManager:
    //  used for any memory allocations
    MemoryManager* const    fMemoryManager;
    SchemaGrammar*          fGrammar;
    XSModel*                fXSModel;

    /* Need a XSNamedMap for each component    top-level?
       that is top level.
	      ATTRIBUTE_DECLARATION     = 1,	   
	      ELEMENT_DECLARATION       = 2,	    
	      TYPE_DEFINITION           = 3,	    
	      ATTRIBUTE_USE             = 4,	   no 
	      ATTRIBUTE_GROUP_DEFINITION= 5,	    
	      MODEL_GROUP_DEFINITION    = 6,	   
	      MODEL_GROUP               = 7,	   no 
	      PARTICLE                  = 8,	   no
	      WILDCARD                  = 9,	   no
	      IDENTITY_CONSTRAINT       = 10,	   no
	      NOTATION_DECLARATION      = 11,	    
	      ANNOTATION                = 12,	   no
	      FACET                     = 13,      no
	      MULTIVALUE_FACET          = 14       no
    */
    XSNamedMap<XSObject>*                   fComponentMap[XSConstants::MULTIVALUE_FACET];
    XSAnnotationList*                       fXSAnnotationList;
    RefHashTableOf<XSObject>*               fHashMap[XSConstants::MULTIVALUE_FACET];
    const XMLCh*                            fSchemaNamespace;
};

inline XSAnnotationList* XSNamespaceItem::getAnnotations()
{
    return fXSAnnotationList;
}

inline const XMLCh *XSNamespaceItem::getSchemaNamespace()
{
    return fSchemaNamespace;
}



XERCES_CPP_NAMESPACE_END

#endif
