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
 * $Log: XSElementDeclaration.hpp,v $
 * Revision 1.9  2003/12/24 17:42:02  knoaman
 * Misc. PSVI updates
 *
 * Revision 1.8  2003/12/01 23:23:26  neilg
 * fix for bug 25118; thanks to Jeroen Witmond
 *
 * Revision 1.7  2003/12/01 20:41:25  neilg
 * fix for infinite loop between XSComplexTypeDefinitions and XSElementDeclarations; from David Cargill
 *
 * Revision 1.6  2003/11/23 16:20:16  knoaman
 * PSVI: pass scope and enclosing type during construction.
 *
 * Revision 1.5  2003/11/21 17:29:53  knoaman
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

#if !defined(XSELEMENTDECLARATION_HPP)
#define XSELEMENTDECLARATION_HPP

#include <xercesc/framework/psvi/XSObject.hpp>
#include <xercesc/framework/psvi/XSNamedMap.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
 * This class describes all properties of a Schema Element Declaration
 * component.
 * This is *always* owned by the validator /parser object from which
 * it is obtained.  
 */

// forward declarations
class XSAnnotation;
class XSComplexTypeDefinition;
class XSIDCDefinition;
class XSTypeDefinition;
class SchemaElementDecl;

class XMLPARSER_EXPORT XSElementDeclaration : public XSObject
{
public:

    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    /**
      * The default constructor 
      *
      * @param  schemaElementDecl
      * @param  typeDefinition
      * @param  substitutionGroupAffiliation
      * @param  annot
      * @param  identityConstraints
      * @param  xsModel
      * @param  elemScope
      * @param  enclosingTypeDefinition
      * @param  manager     The configurable memory manager
      */
    XSElementDeclaration
    (
        SchemaElementDecl* const             schemaElementDecl
        , XSTypeDefinition* const            typeDefinition
        , XSElementDeclaration* const        substitutionGroupAffiliation
        , XSAnnotation* const                annot
        , XSNamedMap<XSIDCDefinition>* const identityConstraints
        , XSModel* const                     xsModel
        , XSConstants::SCOPE                 elemScope = XSConstants::SCOPE_ABSENT
        , XSComplexTypeDefinition* const     enclosingTypeDefinition = 0
        , MemoryManager* const               manager = XMLPlatformUtils::fgMemoryManager
    );

    //@};

    /** @name Destructor */
    //@{
    ~XSElementDeclaration();
    //@}

    //---------------------
    /** @name overridden XSXSObject methods */

    //@{

    /**
     * The name of type <code>NCName</code> of this declaration as defined in 
     * XML Namespaces.
     */
    const XMLCh* getName();

    /**
     *  The [target namespace] of this object, or <code>null</code> if it is 
     * unspecified. 
     */
    const XMLCh* getNamespace();

    /**
     * A namespace schema information item corresponding to the target 
     * namespace of the component, if it's globally declared; or null 
     * otherwise.
     */
    XSNamespaceItem *getNamespaceItem();

    /**
      * Return a unique identifier for a component within this XSModel, to
      * optimize querying.
      * @return id unique for this type of component within this XSModel.
      */
    unsigned int getId() const;

    //@}

    //---------------------
    /** @name XSElementDeclaration methods */

    //@{

    /**
     * [type definition]: either a simple type definition or a complex type 
     * definition. 
     */
    XSTypeDefinition *getTypeDefinition() const;

    /**
     * Optional. One of <code>SCOPE_GLOBAL</code>, <code>SCOPE_LOCAL</code>, 
     * or <code>SCOPE_ABSENT</code>. If the scope is local, then the 
     * <code>enclosingCTDefinition</code> is present. 
     */
    XSConstants::SCOPE getScope() const;

    /**
     * The complex type definition for locally scoped declarations (see 
     * <code>scope</code>). 
     */
    XSComplexTypeDefinition *getEnclosingCTDefinition() const;

    /**
     * [Value constraint]: one of <code>VC_NONE, VC_DEFAULT, VC_FIXED</code>. 
     */
    XSConstants::VALUE_CONSTRAINT getConstraintType() const;

    /**
     * [Value constraint]: the actual value with respect to the [type 
     * definition]. 
     */
    const XMLCh *getConstraintValue();

    /**
     * If nillable is true, then an element may also be valid if it carries 
     * the namespace qualified attribute with local name <code>nil</code> 
     * from namespace <code>http://www.w3.org/2001/XMLSchema-instance</code> 
     * and value <code>true</code> (xsi:nil) even if it has no text or 
     * element content despite a <code>content type</code> which would 
     * otherwise require content. 
     */
    bool getNillable() const;

    /**
     * identity-constraint definitions: a set of constraint definitions. 
     */
    XSNamedMap <XSIDCDefinition> *getIdentityConstraints();

    /**
     * [substitution group affiliation]: optional. A top-level element 
     * definition. 
     */
    XSElementDeclaration *getSubstitutionGroupAffiliation() const;

    /**
     * Convenience method. Check if <code>exclusion</code> is a substitution 
     * group exclusion for this element declaration. 
     * @param exclusion  
     *   <code>DERIVATION_EXTENSION, DERIVATION_RESTRICTION</code> or 
     *   <code>DERIVATION_NONE</code>. Represents final set for the element.
     * @return True if <code>exclusion</code> is a part of the substitution 
     *   group exclusion subset. 
     */
    bool isSubstitutionGroupExclusion(XSConstants::DERIVATION_TYPE exclusion);

    /**
     * [substitution group exclusions]: the returned value is a bit 
     * combination of the subset of {
     * <code>DERIVATION_EXTENSION, DERIVATION_RESTRICTION</code>} or 
     * <code>DERIVATION_NONE</code>. 
     */
    short getSubstitutionGroupExclusions() const;

    /**
     * Convenience method. Check if <code>disallowed</code> is a disallowed 
     * substitution for this element declaration. 
     * @param disallowed {
     *   <code>DERIVATION_SUBSTITUTION, DERIVATION_EXTENSION, DERIVATION_RESTRICTION</code>
     *   } or <code>DERIVATION_NONE</code>. Represents a block set for the 
     *   element.
     * @return True if <code>disallowed</code> is a part of the substitution 
     *   group exclusion subset. 
     */
    bool isDisallowedSubstitution(XSConstants::DERIVATION_TYPE disallowed);

    /**
     * [disallowed substitutions]: the returned value is a bit combination of 
     * the subset of {
     * <code>DERIVATION_SUBSTITUTION, DERIVATION_EXTENSION, DERIVATION_RESTRICTION</code>
     * } corresponding to substitutions disallowed by this 
     * <code>XSElementDeclaration</code> or <code>DERIVATION_NONE</code>. 
     */
    short getDisallowedSubstitutions() const;

    /**
     * {abstract} A boolean. 
     */
    bool getAbstract() const;

    /**
     * Optional. Annotation. 
     */
    XSAnnotation *getAnnotation() const;

    //@}

    //----------------------------------
    /** methods needed by implementation */

    //@{
    /**
      * Set the id to be returned on getId().
      */
    void setId(unsigned int id);

    void setTypeDefinition(XSTypeDefinition* typeDefinition);

    //@}
private:

    void setEnclosingCTDefinition(XSComplexTypeDefinition* const toSet);
    friend class XSObjectFactory;

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XSElementDeclaration(const XSElementDeclaration&);
    XSElementDeclaration & operator=(const XSElementDeclaration &);

protected:

    // -----------------------------------------------------------------------
    //  data members
    // -----------------------------------------------------------------------
    short                         fDisallowedSubstitutions;
    short                         fSubstitutionGroupExclusions;
    unsigned int                  fId;
    XSConstants::SCOPE            fScope;
    SchemaElementDecl*            fSchemaElementDecl;
    XSTypeDefinition*             fTypeDefinition;
    XSComplexTypeDefinition*      fEnclosingTypeDefinition;
    XSElementDeclaration*         fSubstitutionGroupAffiliation;
    XSAnnotation*                 fAnnotation;
    XSNamedMap<XSIDCDefinition>*  fIdentityConstraints;
};

inline XSTypeDefinition* XSElementDeclaration::getTypeDefinition() const
{
    return fTypeDefinition;
}

inline XSNamedMap<XSIDCDefinition>* XSElementDeclaration::getIdentityConstraints()
{
    return fIdentityConstraints;
}

inline XSElementDeclaration* XSElementDeclaration::getSubstitutionGroupAffiliation() const
{
    return fSubstitutionGroupAffiliation;
}

inline short XSElementDeclaration::getSubstitutionGroupExclusions() const
{
    return fSubstitutionGroupExclusions;
}

inline short XSElementDeclaration::getDisallowedSubstitutions() const
{
    return fDisallowedSubstitutions;
}

inline XSAnnotation *XSElementDeclaration::getAnnotation() const
{
    return fAnnotation;
}

inline void XSElementDeclaration::setId(unsigned int id)
{
    fId = id;
}

inline XSConstants::SCOPE XSElementDeclaration::getScope() const
{
    return fScope;
}

inline XSComplexTypeDefinition *XSElementDeclaration::getEnclosingCTDefinition() const
{
    return fEnclosingTypeDefinition;
}

inline void XSElementDeclaration::setTypeDefinition(XSTypeDefinition* typeDefinition)
{
    fTypeDefinition = typeDefinition;
}

inline void XSElementDeclaration::setEnclosingCTDefinition(XSComplexTypeDefinition* const toSet)
{
    fEnclosingTypeDefinition = toSet;
}

XERCES_CPP_NAMESPACE_END

#endif
