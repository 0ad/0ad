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
 * $Log: XSConstants.hpp,v $
 * Revision 1.3  2004/01/29 11:46:30  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.2  2003/11/06 15:30:04  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.1  2003/09/16 14:33:36  neilg
 * PSVI/schema component model classes, with Makefile/configuration changes necessary to build them
 *
 */

#if !defined(XSCONSTANTS_HPP)
#define XSCONSTANTS_HPP

#include <xercesc/util/RefVectorOf.hpp>
#include <xercesc/util/RefArrayVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * This contains constants needed in the schema component model.
 */

// forward definitions for typedefs
class XSAnnotation;
class XSAttributeUse;
class XSFacet;
class XSMultiValueFacet;
class XSNamespaceItem;
class XSParticle;
class XSSimpleTypeDefinition;

// these typedefs are intended to help hide dependence on utility
// classes, as well as to define more intuitive names for commonly
// used concepts.

typedef RefVectorOf <XSAnnotation> XSAnnotationList;
typedef RefVectorOf <XSAttributeUse> XSAttributeUseList;
typedef RefVectorOf <XSFacet> XSFacetList;
typedef RefVectorOf <XSMultiValueFacet> XSMultiValueFacetList;
typedef RefVectorOf <XSNamespaceItem> XSNamespaceItemList;
typedef RefVectorOf <XSParticle> XSParticleList;
typedef RefVectorOf <XSSimpleTypeDefinition> XSSimpleTypeDefinitionList;
typedef RefArrayVectorOf <XMLCh> StringList;

class XMLPARSER_EXPORT XSConstants 
{
public:

    // XML Schema Components
    enum COMPONENT_TYPE {
	    /**
	     * The object describes an attribute declaration.
	     */
	      ATTRIBUTE_DECLARATION     = 1,
	    /**
	     * The object describes an element declaration.
	     */
	      ELEMENT_DECLARATION       = 2,
	    /**
	     * The object describes a complex type or simple type definition.
	     */
	      TYPE_DEFINITION           = 3,
	    /**
	     * The object describes an attribute use definition.
	     */
	      ATTRIBUTE_USE             = 4,
	    /**
	     * The object describes an attribute group definition.
	     */
	      ATTRIBUTE_GROUP_DEFINITION= 5,
	    /**
	     * The object describes a model group definition.
	     */
	      MODEL_GROUP_DEFINITION    = 6,
	    /**
	     * A model group.
	     */
	      MODEL_GROUP               = 7,
	    /**
	     * The object describes a particle.
	     */
	      PARTICLE                  = 8,
	    /**
	     * The object describes a wildcard.
	     */
	      WILDCARD                  = 9,
	    /**
	     * The object describes an identity constraint definition.
	     */
	      IDENTITY_CONSTRAINT       = 10,
	    /**
	     * The object describes a notation declaration.
	     */
	      NOTATION_DECLARATION      = 11,
	    /**
	     * The object describes an annotation.
	     */
	      ANNOTATION                = 12,
	    /**
	     * The object describes a constraining facet.
	     */
	      FACET                     = 13,
	    
	    /**
	     * The object describes enumeration/pattern facets.
	     */
	      MULTIVALUE_FACET           = 14
    };

    // Derivation constants
    enum DERIVATION_TYPE {
	    /**
	     * No constraint is available.
	     */
	     DERIVATION_NONE     = 0,
	    /**
	     * <code>XSTypeDefinition</code> final set or 
	     * <code>XSElementDeclaration</code> disallowed substitution group.
	     */
	     DERIVATION_EXTENSION      = 1,
	    /**
	     * <code>XSTypeDefinition</code> final set or 
	     * <code>XSElementDeclaration</code> disallowed substitution group.
	     */
	     DERIVATION_RESTRICTION    = 2,
	    /**
	     * <code>XSTypeDefinition</code> final set.
	     */
	     DERIVATION_SUBSTITUTION   = 4,
	    /**
	     * <code>XSTypeDefinition</code> final set.
	     */
	     DERIVATION_UNION          = 8,
	    /**
	     * <code>XSTypeDefinition</code> final set.
	     */
	     DERIVATION_LIST           = 16
    };

    // Scope
    enum SCOPE {
	    /**
	     * The scope of a declaration within named model groups or attribute 
	     * groups is <code>absent</code>. The scope of such declaration is 
	     * determined when it is used in the construction of complex type 
	     * definitions. 
	     */
	     SCOPE_ABSENT              = 0,
	    /**
	     * A scope of <code>global</code> identifies top-level declarations. 
	     */
	     SCOPE_GLOBAL              = 1,
	    /**
	     * <code>Locally scoped</code> declarations are available for use only 
	     * within the complex type.
	     */
	     SCOPE_LOCAL               = 2
    };

    // Value Constraint
    enum VALUE_CONSTRAINT {
	    /**
	     * Indicates that the component does not have any value constraint.
	     */
	     VC_NONE          = 0,
	    /**
	     * Indicates that there is a default value constraint.
	     */
	     VC_DEFAULT                = 1,
	    /**
	     * Indicates that there is a fixed value constraint for this attribute.
	     */
	     VC_FIXED                  = 2
    };

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XSConstants();
};

XERCES_CPP_NAMESPACE_END

#endif
