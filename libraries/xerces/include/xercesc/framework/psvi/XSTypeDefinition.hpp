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
 * $Log: XSTypeDefinition.hpp,v $
 * Revision 1.8  2003/12/01 23:23:26  neilg
 * fix for bug 25118; thanks to Jeroen Witmond
 *
 * Revision 1.7  2003/11/25 18:08:31  knoaman
 * Misc. PSVI updates. Thanks to David Cargill.
 *
 * Revision 1.6  2003/11/21 17:34:04  knoaman
 * PSVI update
 *
 * Revision 1.5  2003/11/15 21:19:01  neilg
 * fixes for compilation under gcc
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

#if !defined(XSTYPEDEFINITION_HPP)
#define XSTYPEDEFINITION_HPP

#include <xercesc/framework/psvi/XSObject.hpp>

XERCES_CPP_NAMESPACE_BEGIN

// forward declarations
class XSNamespaceItem;

/**
 * This class represents a complexType or simpleType definition.
 * This is *always* owned by the validator /parser object from which
 * it is obtained.  
 *
 */

class XMLPARSER_EXPORT XSTypeDefinition : public XSObject
{
public:

    enum TYPE_CATEGORY {
        /**
        * This constant value signifies a complex type.
        */
        COMPLEX_TYPE              = 15,
	    /**
	     * This constant value signifies a simple type.
	     */
	    SIMPLE_TYPE               = 16
    };

    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    /** @name Constructors */
    //@{

    /**
      * The default constructor 
      *
      * @param  typeCategory
      * @param  xsBaseType
      * @param  xsModel
      * @param  manager     The configurable memory manager
      */
    XSTypeDefinition
    (
        TYPE_CATEGORY             typeCategory
        , XSTypeDefinition* const xsBaseType
        , XSModel* const          xsModel
        , MemoryManager* const    manager = XMLPlatformUtils::fgMemoryManager
    );

    //@};

    /** @name Destructor */
    //@{
    virtual ~XSTypeDefinition();
    //@}

    //---------------------
    /** @name overloaded XSObject methods */
    //@{

    /**
     * The name of type <code>NCName</code> of this declaration as defined in 
     * XML Namespaces.
     */
    virtual const XMLCh* getName() = 0;

    /**
     *  The [target namespace] of this object, or <code>null</code> if it is 
     * unspecified. 
     */
    virtual const XMLCh* getNamespace() = 0;

    /**
     * A namespace schema information item corresponding to the target 
     * namespace of the component, if it's globally declared; or null 
     * otherwise.
     */
    virtual XSNamespaceItem *getNamespaceItem() = 0;

    //@}

    //---------------------
    /** @name XSTypeDefinition methods */

    //@{

    /**
     * Return whether this type definition is a simple type or complex type.
     */
    TYPE_CATEGORY getTypeCategory() const;

    /**
     * {base type definition}: either a simple type definition or a complex 
     * type definition. 
     */
    virtual XSTypeDefinition *getBaseType() = 0;

    /**
     * {final}. For complex type definition it is a subset of {extension, 
     * restriction}. For simple type definition it is a subset of 
     * {extension, list, restriction, union}. 
     * @param toTest       Extension, restriction, list, union constants 
     *   (defined in <code>XSObject</code>). 
     * @return True if toTest is in the final set, otherwise false.
     */
    bool isFinal(short toTest);

    /**
     * For complex types the returned value is a bit combination of the subset 
     * of {<code>DERIVATION_EXTENSION, DERIVATION_RESTRICTION</code>} 
     * corresponding to <code>final</code> set of this type or 
     * <code>DERIVATION_NONE</code>. For simple types the returned value is 
     * a bit combination of the subset of { 
     * <code>DERIVATION_RESTRICTION, DERIVATION_EXTENSION, DERIVATION_UNION, DERIVATION_LIST</code>
     * } corresponding to <code>final</code> set of this type or 
     * <code>DERIVATION_NONE</code>. 
     */
    short getFinal() const;

    /**
     *  A boolean that specifies if the type definition is 
     * anonymous. Convenience attribute. 
     */
    virtual bool getAnonymous() const = 0;

    /**
     * Convenience method: check if this type is derived from the given 
     * <code>ancestorType</code>. 
     * @param ancestorType  An ancestor type definition. 
     * @return  Return true if this type is derived from 
     *   <code>ancestorType</code>.
     */
    virtual bool derivedFromType(const XSTypeDefinition* const ancestorType) = 0;

    /**
     * Convenience method: check if this type is derived from the given 
     * ancestor type. 
     * @param typeNamespace  An ancestor type namespace. 
     * @param name  An ancestor type name. 
     * @return  Return true if this type is derived from 
     *   the ancestor defined by <code>typeNamespace</code> and <code>name</code>.
     */
    bool derivedFrom(const XMLCh* typeNamespace, 
                               const XMLCh* name);

    //@}

    //----------------------------------
    /** methods needed by implementation */

    //@{

    //@}
private:

    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XSTypeDefinition(const XSTypeDefinition&);
    XSTypeDefinition & operator=(const XSTypeDefinition &);

protected:

    // -----------------------------------------------------------------------
    //  data members
    // -----------------------------------------------------------------------
    // fTypeCategory
    //  whether this is a simpleType or complexType
    // fFinal
    //  the final properties which is set by the derived class.
    TYPE_CATEGORY     fTypeCategory;
    short             fFinal;
    XSTypeDefinition* fBaseType; // owned by XSModel
};

inline XSTypeDefinition::TYPE_CATEGORY XSTypeDefinition::getTypeCategory() const
{
    return fTypeCategory;
}

inline short XSTypeDefinition::getFinal() const
{
    return fFinal;
}


XERCES_CPP_NAMESPACE_END

#endif
