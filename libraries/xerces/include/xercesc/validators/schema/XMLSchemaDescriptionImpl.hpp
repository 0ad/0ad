/*
 * Copyright 1999-2004 The Apache Software Foundation.
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
 * $Log$
 * Revision 1.4  2004/09/08 13:56:57  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.3  2003/10/14 15:22:28  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.2  2003/07/31 17:14:27  peiyongz
 * Grammar embed grammar description
 *
 * Revision 1.1  2003/06/20 18:40:29  peiyongz
 * Stateless Grammar Pool :: Part I
 *
 * $Id: XMLSchemaDescriptionImpl.hpp 176026 2004-09-08 13:57:07Z peiyongz $
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
