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
 * $Id: XMLDTDDescription.hpp 191054 2005-06-17 02:56:35Z jberry $
 */

#if !defined(XMLDTDDESCRIPTION_HPP)
#define XMLDTDDESCRIPTION_HPP

#include <xercesc/framework/XMLGrammarDescription.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLPARSER_EXPORT XMLDTDDescription : public XMLGrammarDescription
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
    virtual ~XMLDTDDescription();
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
        return Grammar::DTDGrammarType;
    }
    //@}

    // -----------------------------------------------------------------------
    /** @name The DTDDescription Interface */
    // -----------------------------------------------------------------------
    //@{      
    /**
      * Getter
      *
      */
    virtual const XMLCh*          getRootName() const = 0;
    virtual const XMLCh*          getSystemId() const {return 0;};

    /**
      * Setter
      *
      */
    virtual void                  setRootName(const XMLCh* const) = 0;
    virtual void                  setSystemId(const XMLCh* const) {};
    //@}
    
    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(XMLDTDDescription)

protected :
    // -----------------------------------------------------------------------
    /**  Hidden Constructors */
    // -----------------------------------------------------------------------
    //@{
    XMLDTDDescription(MemoryManager* const memMgr = XMLPlatformUtils::fgMemoryManager);
    //@}

private :
    // -----------------------------------------------------------------------
    /** name  Unimplemented copy constructor and operator= */
    // -----------------------------------------------------------------------
    //@{
    XMLDTDDescription(const XMLDTDDescription& );
    XMLDTDDescription& operator=(const XMLDTDDescription& );
    //@}

};


XERCES_CPP_NAMESPACE_END

#endif
