/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2001 The Apache Software Foundation.  All rights
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
 * $Log: DTDElementDecl.hpp,v $
 * Revision 1.9  2004/01/29 11:52:30  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.8  2003/12/12 18:35:44  peiyongz
 * getObjectType()
 *
 * Revision 1.7  2003/10/10 16:24:51  peiyongz
 * Implementation of Serialization/Deserialization
 *
 * Revision 1.6  2003/10/08 21:33:18  peiyongz
 * Synchronize ContentSpec/ContentModel/FormattedModel
 *
 * Revision 1.5  2003/05/16 21:43:19  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.4  2003/05/15 18:54:50  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2003/01/29 19:46:40  gareth
 * added DOMTypeInfo API
 *
 * Revision 1.2  2002/11/04 14:50:40  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:43  peiyongz
 * sane_include
 *
 * Revision 1.12  2001/09/05 20:49:10  knoaman
 * Fix for complexTypes with mixed content model.
 *
 * Revision 1.11  2001/08/21 16:06:11  tng
 * Schema: Unique Particle Attribution Constraint Checking.
 *
 * Revision 1.10  2001/05/11 13:27:08  tng
 * Copyright update.
 *
 * Revision 1.9  2001/04/19 18:17:20  tng
 * Schema: SchemaValidator update, and use QName in Content Model
 *
 * Revision 1.8  2001/03/21 21:56:19  tng
 * Schema: Add Schema Grammar, Schema Validator, and split the DTDValidator into DTDValidator, DTDScanner, and DTDGrammar.
 *
 * Revision 1.7  2001/03/21 19:29:43  tng
 * Schema: Content Model Updates, by Pei Yong Zhang.
 *
 * Revision 1.6  2001/02/26 19:29:21  tng
 * Schema: add virtual method getURI(), getContentSpec and setContenSpec in XMLElementDecl, and DTDElementDecl.
 *
 * Revision 1.5  2001/02/26 19:22:02  tng
 * Schema: add parameter prefix in findElem and findAttr.
 *
 * Revision 1.4  2000/02/24 20:16:49  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.3  2000/02/09 21:42:37  abagchi
 * Copyright swat
 *
 * Revision 1.2  1999/11/23 01:51:04  rahulj
 * Cannot use class qualifier in class defn. CC under HPUX is happy.
 *
 * Revision 1.1.1.1  1999/11/09 01:03:32  twl
 * Initial checkin
 *
 * Revision 1.3  1999/11/08 20:45:40  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(DTDELEMENTDECL_HPP)
#define DTDELEMENTDECL_HPP

#include <xercesc/util/RefHashTableOf.hpp>
#include <xercesc/util/QName.hpp>
#include <xercesc/framework/XMLElementDecl.hpp>
#include <xercesc/framework/XMLContentModel.hpp>
#include <xercesc/validators/DTD/DTDAttDef.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class ContentSpecNode;
class DTDAttDefList;


//
//  This class is a derivative of the basic element decl. This one implements
//  the virtuals so that they work for a DTD. THe big difference is that
//  they don't live in any URL in the DTD. The names are just stored as full
//  QNames, so they are not split out and element decls don't live within
//  URL namespaces or anything like that.
//

class VALIDATORS_EXPORT DTDElementDecl : public XMLElementDecl
{
public :
    // -----------------------------------------------------------------------
    //  Class specific types
    //
    //  ModelTypes
    //      Indicates the type of content model that an element has. This
    //      indicates how the content model is represented and validated.
    // -----------------------------------------------------------------------
    enum ModelTypes
    {
        Empty
        , Any
        , Mixed_Simple
        , Children

        , ModelTypes_Count
    };


    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    DTDElementDecl(MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);
    DTDElementDecl
    (
          const XMLCh* const   elemRawName
        , const unsigned int   uriId
        , const ModelTypes     modelType
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    DTDElementDecl
    (
          QName* const         elementName
        , const ModelTypes     modelType = Any
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    ~DTDElementDecl();


    // -----------------------------------------------------------------------
    //  The virtual element decl interface
    // -----------------------------------------------------------------------
    virtual XMLAttDef* findAttr
    (
        const   XMLCh* const    qName
        , const unsigned int    uriId
        , const XMLCh* const    baseName
        , const XMLCh* const    prefix
        , const LookupOpts      options
        ,       bool&           wasAdded
    )   const;
    virtual XMLAttDefList& getAttDefList() const;
    virtual CharDataOpts getCharDataOpts() const;
    virtual bool hasAttDefs() const;
    virtual bool resetDefs();
    virtual const ContentSpecNode* getContentSpec() const;
    virtual ContentSpecNode* getContentSpec();
    virtual void setContentSpec(ContentSpecNode* toAdopt);
    virtual XMLContentModel* getContentModel();
    virtual void setContentModel(XMLContentModel* const newModelToAdopt);
    virtual const XMLCh* getFormattedContentModel ()   const;

    // -----------------------------------------------------------------------
    // Support keyed collections
    //
    // This method allows objects of this type be placed into one of the
    // standard keyed collections. This method will return the full name of
    // the element, which will vary depending upon the type of the grammar.
    // -----------------------------------------------------------------------
    const XMLCh* getKey() const;

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    const DTDAttDef* getAttDef(const XMLCh* const attName) const;
    DTDAttDef* getAttDef(const XMLCh* const attName);
    ModelTypes getModelType() const;

    const XMLCh* getDOMTypeInfoName() const;

    const XMLCh* getDOMTypeInfoUri() const;

    // -----------------------------------------------------------------------
    //  Setter methods
    // -----------------------------------------------------------------------
    void addAttDef(DTDAttDef* const toAdd);
    void setModelType(const DTDElementDecl::ModelTypes toSet);

    /***
     * Support for Serialization/De-serialization
     ***/
    DECL_XSERIALIZABLE(DTDElementDecl)

    virtual XMLElementDecl::objectType  getObjectType() const;

private :
    // -----------------------------------------------------------------------
    //  Private helper methods
    // -----------------------------------------------------------------------
    void faultInAttDefList() const;
    XMLContentModel* createChildModel() ;
    XMLContentModel* makeContentModel() ;
    XMLCh* formatContentModel () const ;

    // -----------------------------------------------------------------------
    // Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    DTDElementDecl(const DTDElementDecl &);
    DTDElementDecl& operator = (const  DTDElementDecl&);

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fAttDefs
    //      The list of attributes that are defined for this element. Each
    //      element is its own little 'namespace' for attributes, so each
    //      element maintains its own list of owned attribute defs. It is
    //      faulted in when an attribute is actually added.
    //
    //  fAttList
    //      We have to return a view of our att defs via the abstract view
    //      that the scanner understands. It may or may not ever be asked
    //      for so we fault it in as needed.
    //
    //  fContentSpec
    //      This is the content spec for the node. It contains the original
    //      content spec that was read from the DTD, as a tree of nodes. This
    //      one is always set up, and is used to build the fContentModel
    //      version if we are validating.
    //
    //  fModelType
    //      The content model type of this element. This tells us what kind
    //      of content model to create.
    //
    //  fContentModel
    //      The content model object for this element. It is stored here via
    //      its abstract interface.
    //
    //  fFormattedModel
    //      This is a faulted in member. When the outside world asks for
    //      our content model as a string, we format it and fault it into
    //      this field (to avoid doing the formatted over and over.)
    // -----------------------------------------------------------------------
    RefHashTableOf<DTDAttDef>*  fAttDefs;
    DTDAttDefList*              fAttList;
    ContentSpecNode*            fContentSpec;
    ModelTypes                  fModelType;
    XMLContentModel*            fContentModel;
    XMLCh*                      fFormattedModel;
};

// ---------------------------------------------------------------------------
//  DTDElementDecl: XMLElementDecl virtual interface implementation
// ---------------------------------------------------------------------------
inline ContentSpecNode* DTDElementDecl::getContentSpec()
{
    return fContentSpec;
}

inline const ContentSpecNode* DTDElementDecl::getContentSpec() const
{
    return fContentSpec;
}

inline XMLContentModel* DTDElementDecl::getContentModel()
{
    if (!fContentModel)
        fContentModel = makeContentModel();
    return fContentModel;
}

inline void
DTDElementDecl::setContentModel(XMLContentModel* const newModelToAdopt)
{
    delete fContentModel;
    fContentModel = newModelToAdopt;

    // reset formattedModel
    if (fFormattedModel)
    {
        getMemoryManager()->deallocate(fFormattedModel);
        fFormattedModel = 0;
    }
}

// ---------------------------------------------------------------------------
//  DTDElementDecl: Miscellaneous methods
// ---------------------------------------------------------------------------
inline const XMLCh* DTDElementDecl::getKey() const
{
    return getFullName();
}

// ---------------------------------------------------------------------------
//  DTDElementDecl: Getter methods
// ---------------------------------------------------------------------------
inline DTDElementDecl::ModelTypes DTDElementDecl::getModelType() const
{
    return fModelType;
}

inline const XMLCh* DTDElementDecl::getDOMTypeInfoName() const {
    return 0;
}

inline const XMLCh* DTDElementDecl::getDOMTypeInfoUri() const {
    return 0;
}

// ---------------------------------------------------------------------------
//  DTDElementDecl: Setter methods
// ---------------------------------------------------------------------------
inline void
DTDElementDecl::setModelType(const DTDElementDecl::ModelTypes toSet)
{
    fModelType = toSet;
}

XERCES_CPP_NAMESPACE_END

#endif
