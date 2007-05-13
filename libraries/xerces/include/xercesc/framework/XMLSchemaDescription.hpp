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
 * Revision 1.4  2004/09/08 13:55:59  peiyongz
 * Apache License Version 2.0
 *
 * Revision 1.3  2003/10/14 15:17:47  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.2  2003/07/31 17:03:19  peiyongz
 * locationHint incrementally added
 *
 * Revision 1.1  2003/06/20 18:37:39  peiyongz
 * Stateless Grammar Pool :: Part I
 *
 * $Id: XMLSchemaDescription.hpp 176026 2004-09-08 13:57:07Z peiyongz $
 *
 */

#if !defined(XMLSCHEMADESCRIPTION_HPP)
#define XMLSCHEMADESCRIPTION_HPP

#include <xercesc/framework/XMLGrammarDescription.hpp>
#include <xercesc/util/RefArrayVectorOf.hpp>

XERCES_CPP_NAMESPACE_BEGIN

typedef const XMLCh* const LocationHint;

class XMLPARSER_EXPORT XMLSchemaDescription : public XMLGrammarDescription
{
public :
    // -----------------------------------------------------------------------
    /** @name Virtual destructor for derived classes */
    // -----------------------------------------------------------------------
    //@{
    /**
      * virtual destructor
      *
      */
    virtual ~XMLSchemaDescription();
    //@}

    // -----------------------------------------------------------------------
    /** @name Implementation of Grammar Description Interface */
    // -----------------------------------------------------------------------
    //@{     
    /**
      * getGrammarType
      *
      */
    virtual Grammar::GrammarType   getGrammarType() const
    {
        return Grammar::SchemaGrammarType;
    }
    //@}

    // -----------------------------------------------------------------------
    /** @name The SchemaDescription Interface */
    // -----------------------------------------------------------------------
    //@{

    enum ContextType 
         {
            CONTEXT_INCLUDE,
            CONTEXT_REDEFINE,
            CONTEXT_IMPORT,
            CONTEXT_PREPARSE,
            CONTEXT_INSTANCE,
            CONTEXT_ELEMENT,
            CONTEXT_ATTRIBUTE,
            CONTEXT_XSITYPE,
            CONTEXT_UNKNOWN
         };

    /**
      * getContextType
      *
      */	
    virtual ContextType                getContextType() const = 0;

    /**
      * getTargetNamespace
      *
      */	
    virtual const XMLCh*               getTargetNamespace() const = 0;

    /**
      * getLocationHints
      *
      */	
    virtual RefArrayVectorOf<XMLCh>*   getLocationHints() const = 0;

    /**
      * getTriggeringComponent
      *
      */	
    virtual const QName*               getTriggeringComponent() const = 0;

    /**
      * getenclosingElementName
      *
      */	
    virtual const QName*               getEnclosingElementName() const = 0;

    /**
      * getAttributes
      *
      */	
    virtual const XMLAttDef*           getAttributes() const = 0;

    /**
      * setContextType
      *
      */	
    virtual void                       setContextType(ContextType) = 0;

    /**
      * setTargetNamespace
      *
      */	
    virtual void                       setTargetNamespace(const XMLCh* const) = 0;

    /**
      * setLocationHints
      *
      */	
    virtual void                       setLocationHints(const XMLCh* const) = 0;

    /**
      * setTriggeringComponent
      *
      */	
    virtual void                       setTriggeringComponent(QName* const) = 0;

    /**
      * getenclosingElementName
      *
      */	
    virtual void                       setEnclosingElementName(QName* const) = 0;

    /**
      * setAttributes
      *
      */	
    virtual void                       setAttributes(XMLAttDef* const) = 0;
    //@}	          
	          
    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLSchemaDescription)

protected :
    // -----------------------------------------------------------------------
    /**  Hidden Constructors */
    // -----------------------------------------------------------------------
    //@{
    XMLSchemaDescription(MemoryManager* const memMgr = XMLPlatformUtils::fgMemoryManager);
    //@}

private :
    // -----------------------------------------------------------------------
    /** name  Unimplemented copy constructor and operator= */
    // -----------------------------------------------------------------------
    //@{
    XMLSchemaDescription(const XMLSchemaDescription& );
    XMLSchemaDescription& operator=(const XMLSchemaDescription& );
    //@}

};


XERCES_CPP_NAMESPACE_END

#endif
