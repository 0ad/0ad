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
 * $Log: XSModel.hpp,v $
 * Revision 1.10  2003/12/24 17:42:02  knoaman
 * Misc. PSVI updates
 *
 * Revision 1.9  2003/11/26 16:12:23  knoaman
 * Add a method to return the XSObject mapped to a schema grammar component
 *
 * Revision 1.8  2003/11/21 22:34:45  neilg
 * More schema component model implementation, thanks to David Cargill.
 * In particular, this cleans up and completes the XSModel, XSNamespaceItem,
 * XSAttributeDeclaration and XSAttributeGroup implementations.
 *
 * Revision 1.7  2003/11/21 17:25:09  knoaman
 * Use XSObjectFactory to create various components.
 *
 * Revision 1.6  2003/11/15 21:18:39  neilg
 * fixes for compilation under gcc
 *
 * Revision 1.5  2003/11/14 22:47:53  neilg
 * fix bogus log message from previous commit...
 *
 * Revision 1.4  2003/11/14 22:33:30  neilg
 * Second phase of schema component model implementation.  
 * Implement XSModel, XSNamespaceItem, and the plumbing necessary
 * to connect them to the other components.
 * Thanks to David Cargill.
 *
 * Revision 1.3  2003/11/06 15:30:04  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.2  2003/10/10 18:37:51  neilg
 * update XSModel and XSObject interface so that IDs can be used to query components in XSModels, and so that those IDs can be recovered from components
 *
 * Revision 1.1  2003/09/16 14:33:36  neilg
 * PSVI/schema component model classes, with Makefile/configuration changes necessary to build them
 *
 */

#if !defined(XSMODEL_HPP)
#define XSMODEL_HPP

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/psvi/XSObject.hpp>
#include <xercesc/framework/psvi/XSNamedMap.hpp>

#include <xercesc/util/ValueVectorOf.hpp>
#include <xercesc/validators/schema/SchemaElementDecl.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * This class contains all properties of the Schema infoitem as determined
 * after an entire validation episode.  That is, it contains all the properties
 * of all the Schema Namespace Information objects that went into
 * the validation episode.
 * Since it is not like other components, it  does not 
 * inherit from the XSObject interface.
 * This is *always* owned by the validator /parser object from which
 * it is obtained.  It is designed to be subclassed; subclasses will
 * specify under what conditions it may be relied upon to have meaningful contents.
 */

// forward declarations
class Grammar;
class XMLGrammarPool;
class XSAnnotation;
class XSAttributeDeclaration;
class XSAttributeGroupDefinition;
class XSElementDeclaration;
class XSModelGroupDefinition;
class XSNamespaceItem;
class XSNotationDeclaration;
class XSTypeDefinition;
class XSObjectFactory;

class XMLPARSER_EXPORT XSModel : public XMemory
{
public:

    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    /**
      * The constructor to be used when a grammar pool contains all needed info
      * @param grammarPool  the grammar pool containing the underlying data structures
      * @param manager      The configurable memory manager
      */
    XSModel( XMLGrammarPool *grammarPool
                , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /**
      * The constructor to be used when the XSModel must represent all
      * components in the union of an existing XSModel and a newly-created
      * Grammar(s) from the GrammarResolver
      *
      * @param baseModel  the XSModel upon which this one is based
      * @param grammarResolver  the grammar(s) whose components are to be merged
      * @param manager     The configurable memory manager
      */
    XSModel( XSModel *baseModel
                , GrammarResolver *grammarResolver
                , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    //@};

    /** @name Destructor */
    //@{
    ~XSModel();
    //@}

    //---------------------
    /** @name XSModel methods */

    //@{

    /**
     * Convenience method. Returns a list of all namespaces that belong to 
     * this schema. The value <code>null</code> is not a valid namespace 
     * name, but if there are components that don't have a target namespace, 
     * <code>null</code> is included in this list. 
     */
    StringList *getNamespaces();

    /**
     * A set of namespace schema information information items ( of type 
     * <code>XSNamespaceItem</code>), one for each namespace name which 
     * appears as the target namespace of any schema component in the schema 
     * used for that assessment, and one for absent if any schema component 
     * in the schema had no target namespace. For more information see 
     * schema information. 
     */
    XSNamespaceItemList *getNamespaceItems();

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
     * Convenience method. Returns a list of top-level component declarations 
     * that are defined within the specified namespace, i.e. element 
     * declarations, attribute declarations, etc. 
     * @param objectType The type of the declaration, i.e. 
     *   <code>ELEMENT_DECLARATION</code>.
     * @param compNamespace The namespace to which declaration belongs or 
     *   <code>null</code> (for components with no target namespace).
     * @return A list of top-level definitions of the specified type in 
     *   <code>objectType</code> and defined in the specified 
     *   <code>namespace</code> or <code>null</code>. 
     */
    XSNamedMap<XSObject> *getComponentsByNamespace(XSConstants::COMPONENT_TYPE objectType, 
                                               const XMLCh *compNamespace);

    /**
     *  [annotations]: a set of annotations.
     */
    XSAnnotationList *getAnnotations();

    /**
     * Convenience method. Returns a top-level element declaration. 
     * @param name The name of the declaration.
     * @param compNamespace The namespace of the declaration, null if absent.
     * @return A top-level element declaration or <code>null</code> if such 
     *   declaration does not exist. 
     */
    XSElementDeclaration *getElementDeclaration(const XMLCh *name
            , const XMLCh *compNamespace);

    /**
     * Convenience method. Returns a top-level attribute declaration. 
     * @param name The name of the declaration.
     * @param compNamespace The namespace of the declaration, null if absent.
     * @return A top-level attribute declaration or <code>null</code> if such 
     *   declaration does not exist. 
     */
    XSAttributeDeclaration *getAttributeDeclaration(const XMLCh *name
            , const XMLCh *compNamespace);

    /**
     * Convenience method. Returns a top-level simple or complex type 
     * definition. 
     * @param name The name of the definition.
     * @param compNamespace The namespace of the declaration, null if absent.
     * @return An <code>XSTypeDefinition</code> or <code>null</code> if such 
     *   definition does not exist. 
     */
    XSTypeDefinition *getTypeDefinition(const XMLCh *name
            , const XMLCh *compNamespace);

    /**
     * Convenience method. Returns a top-level attribute group definition. 
     * @param name The name of the definition.
     * @param compNamespace The namespace of the declaration, null if absent.
     * @return A top-level attribute group definition or <code>null</code> if 
     *   such definition does not exist. 
     */
    XSAttributeGroupDefinition *getAttributeGroup(const XMLCh *name
            , const XMLCh *compNamespace);

    /**
     * Convenience method. Returns a top-level model group definition. 
     * @param name The name of the definition.
     * @param compNamespace The namespace of the declaration, null if absent.
     * @return A top-level model group definition definition or 
     *   <code>null</code> if such definition does not exist. 
     */
    XSModelGroupDefinition *getModelGroupDefinition(const XMLCh *name
            , const XMLCh *compNamespace);

    /**
     * Convenience method. Returns a top-level notation declaration. 
     * @param name The name of the declaration.
     * @param compNamespace The namespace of the declaration, null if absent.
     * @return A top-level notation declaration or <code>null</code> if such 
     *   declaration does not exist. 
     */
    XSNotationDeclaration *getNotationDeclaration(const XMLCh *name
            , const XMLCh *compNamespace);

    /**
      * Optional.  Return a component given a component type and a unique Id.  
      * May not be supported for all component types.
      * @param compId unique Id of the component within its type
      * @param compType type of the component
      * @return the component of the given type with the given Id, or 0
      * if no such component exists or this is unsupported for
      * this type of component.
      */
    XSObject *getXSObjectById(unsigned int  compId
                , XSConstants::COMPONENT_TYPE compType);

    //@}

    //----------------------------------
    /** methods needed by implementation */

    //@{
    XMLStringPool*  getURIStringPool();

    XSNamespaceItem* getNamespaceItem(const XMLCh* const key);

    /**
      * Get the XSObject (i.e. XSElementDeclaration) that corresponds to
      * to a schema grammar component (i.e. SchemaElementDecl)
      * @param key schema component object
      *
      * @return the corresponding XSObject
      */
    XSObject* getXSObject(void* key);

    //@}
private:

    // -----------------------------------------------------------------------
    //  Helper methods
    // -----------------------------------------------------------------------
    void addGrammarToXSModel
    (
        XSNamespaceItem* namespaceItem
    );
    void addS4SToXSModel
    (
        XSNamespaceItem* const namespaceItem
        , RefHashTableOf<DatatypeValidator>* const builtInDV
    );
    void addComponentToNamespace
    (
         XSNamespaceItem* const namespaceItem
         , XSObject* const component
         , int componentIndex
         , bool addToXSModel = true
    );

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XSModel(const XSModel&);
    XSModel & operator=(const XSModel &);

protected:
    friend class XSObjectFactory;

    // -----------------------------------------------------------------------
    //  data members
    // -----------------------------------------------------------------------
    // fMemoryManager:
    //  used for any memory allocations
    MemoryManager* const                    fMemoryManager;
 
    StringList*                             fNamespaceStringList;
    XSNamespaceItemList*                    fXSNamespaceItemList;

    RefVectorOf<XSElementDeclaration>*      fElementDeclarationVector;
    RefVectorOf<XSAttributeDeclaration>*    fAttributeDeclarationVector;

    /* Need a XSNamedMap for each component    top-level?
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
    XMLStringPool*                          fURIStringPool;
    XSAnnotationList*                       fXSAnnotationList;
    RefHashTableOf<XSNamespaceItem>*        fHashNamespace;
    XSObjectFactory*                        fObjFactory;
    RefVectorOf<XSNamespaceItem>*           fDeleteNamespace;
    XSModel*                                fParent;
    bool                                    fDeleteParent;
    bool                                    fAddedS4SGrammar;
};

inline XMLStringPool*  XSModel::getURIStringPool()
{
    return fURIStringPool;
}

inline StringList *XSModel::getNamespaces()
{
    return fNamespaceStringList;
}

inline XSNamespaceItemList *XSModel::getNamespaceItems()
{
    return fXSNamespaceItemList;
}

XERCES_CPP_NAMESPACE_END

#endif
