/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2003 The Apache Software Foundation.  All rights
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
 * $Log: XMLSchemaDescriptionImpl.hpp,v $
 * Revision 1.3  2003/10/14 15:22:28  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.2  2003/07/31 17:14:27  peiyongz
 * Grammar embed grammar description
 *
 * Revision 1.1  2003/06/20 18:40:29  peiyongz
 * Stateless Grammar Pool :: Part I
 *
 * $Id: XMLSchemaDescriptionImpl.hpp,v 1.3 2003/10/14 15:22:28 peiyongz Exp $
 *
 */

#if !defined(XMLSchemaDescriptionImplIMPL_HPP)
#define XMLSchemaDescriptionImplIMPL_HPP

#include <xercesc/framework/XMLSchemaDescription.hpp>
#include <xercesc/util/RefVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLAttDefs;

class XMLPARSER_EXPORT XMLSchemaDescriptionImpl : public XMLSchemaDescription
{
public :
    // -----------------------------------------------------------------------
    /** @name constructor and destructor */
    // -----------------------------------------------------------------------
    //@{
    XMLSchemaDescriptionImpl(
                             const XMLCh* const   targetNamespace 
                           , MemoryManager* const memMgr
                             );

    ~XMLSchemaDescriptionImpl();
    //@}

    // -----------------------------------------------------------------------
    /** @name Implementation of GrammarDescription Interface */
    // -----------------------------------------------------------------------
    //@{
    /**
      * getGrammarKey
      *
      */
    virtual const XMLCh*           getGrammarKey() const;
    //@}

    // -----------------------------------------------------------------------
    /** @name Implementation of SchemaDescription Interface */
    // -----------------------------------------------------------------------
    //@{

    /**
      * getContextType
      *
      */	
    virtual ContextType            getContextType() const;

    /**
      * getTargetNamespace
      *
      */	
    virtual const XMLCh*           getTargetNamespace() const;

    /**
      * getLocationHints
      *
      */	
    virtual RefArrayVectorOf<XMLCh>*   getLocationHints() const;

    /**
      * getTriggeringComponent
      *
      */	
    virtual const QName*           getTriggeringComponent() const;

    /**
      * getenclosingElementName
      *
      */	
    virtual const QName*           getEnclosingElementName() const;

    /**
      * getAttributes
      *
      */	
    virtual const XMLAttDef*       getAttributes() const;

    /**
      * setContextType
      *
      */	
    virtual void                   setContextType(ContextType);

    /**
      * setTargetNamespace
      *
      */	
    virtual void                   setTargetNamespace(const XMLCh* const);

    /**
      * setLocationHints
      *
      */	
    virtual void                   setLocationHints(const XMLCh* const);

    /**
      * setTriggeringComponent
      *
      */	
    virtual void                   setTriggeringComponent(QName* const);

    /**
      * getenclosingElementName
      *
      */	
    virtual void                   setEnclosingElementName(QName* const);

    /**
      * setAttributes
      *
      */	
    virtual void                   setAttributes(XMLAttDef* const);          
    //@}

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLSchemaDescriptionImpl)

    XMLSchemaDescriptionImpl(MemoryManager* const memMgr = XMLPlatformUtils::fgMemoryManager);

private :
    // -----------------------------------------------------------------------
    /** name  Unimplemented copy constructor and operator= */
    // -----------------------------------------------------------------------
    //@{
    XMLSchemaDescriptionImpl(const XMLSchemaDescriptionImpl& );
    XMLSchemaDescriptionImpl& operator=(const XMLSchemaDescriptionImpl& );
    //@}

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  All data member in this implementation are owned to out survive
    //  parser. Except for fNamespace which is replicated upon set, the
    //  rest shall be created by the embedded memoryManager.
    //
    //  fContextType  
    //
    //  fNamespace            owned
    //
    //  fLocationHints        owned
    //
    //  fTriggeringComponent  owned
    //
    //  fEnclosingElementName owned
    //                       
    //  fAttributes           referenced
    //
    // -----------------------------------------------------------------------

    XMLSchemaDescription::ContextType      fContextType;
    const XMLCh*                           fNamespace;
    RefArrayVectorOf<XMLCh>*               fLocationHints;
    const QName*                           fTriggeringComponent;
    const QName*                           fEnclosingElementName;
    const XMLAttDef*                       fAttributes; 

};


XERCES_CPP_NAMESPACE_END

#endif
