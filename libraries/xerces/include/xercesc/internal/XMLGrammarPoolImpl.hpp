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
 * $Log: XMLGrammarPoolImpl.hpp,v $
 * Revision 1.14  2003/12/19 23:02:43  cargilld
 * Fix compiler messages on OS390.
 *
 * Revision 1.13  2003/11/21 22:38:50  neilg
 * Enable grammar pools and grammar resolvers to manufacture
 * XSModels.  This also cleans up handling in the
 * parser classes by eliminating the need to tell
 * the grammar pool that schema compoments need to be produced.
 * Thanks to David Cargill.
 *
 * Revision 1.12  2003/11/14 22:34:20  neilg
 * removed methods made unnecessary by new XSModel implementation design; thanks to David Cargill
 *
 * Revision 1.11  2003/11/06 21:53:52  neilg
 * update grammar pool interface so that cacheGrammar(Grammar) can tell the caller whether the grammar was accepted.  Also fix some documentation errors.
 *
 * Revision 1.10  2003/11/06 15:30:06  neilg
 * first part of PSVI/schema component model implementation, thanks to David Cargill.  This covers setting the PSVIHandler on parser objects, as well as implementing XSNotation, XSSimpleTypeDefinition, XSIDCDefinition, and most of XSWildcard, XSComplexTypeDefinition, XSElementDeclaration, XSAttributeDeclaration and XSAttributeUse.
 *
 * Revision 1.9  2003/11/05 18:19:45  peiyongz
 * Documentation update
 *
 * Revision 1.8  2003/10/29 16:16:08  peiyongz
 * GrammarPool' serialization/deserialization support
 *
 * Revision 1.7  2003/10/10 18:36:41  neilg
 * update XMLGrammarPool default implementation to reflect recent modifications to the base interface.
 *
 * Revision 1.6  2003/10/09 13:54:25  neilg
 * modify grammar pool implementation to that, once locked, a thread-safe StringPool is used
 *
 * Revision 1.5  2003/09/16 18:30:54  neilg
 * make Grammar pool be responsible for creating and owning URI string pools.  This is one more step towards having grammars be independent of the parsers involved in their creation
 *
 * Revision 1.4  2003/09/02 08:59:02  gareth
 * Added API to get enumerator of grammars.
 *
 * Revision 1.3  2003/07/31 17:05:03  peiyongz
 * Grammar embed Grammar Description
 * using getGrammar(URI)
 * update GrammarDescription info
 *
 * Revision 1.2  2003/06/23 21:06:21  peiyongz
 * to solve unresolved symbol on Solaris
 *
 * Revision 1.1  2003/06/20 18:38:39  peiyongz
 * Stateless Grammar Pool :: Part I
 *
 * $Id: XMLGrammarPoolImpl.hpp,v 1.14 2003/12/19 23:02:43 cargilld Exp $
 *
 */

#if !defined(XMLGrammarPoolImplIMPL_HPP)
#define XMLGrammarPoolImplIMPL_HPP

#include <xercesc/framework/XMLGrammarPool.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLSynchronizedStringPool;

class XMLUTIL_EXPORT XMLGrammarPoolImpl : public XMLGrammarPool
{
public :
    // -----------------------------------------------------------------------
    /** @name constructor and destructor */
    // -----------------------------------------------------------------------
    //@{

    XMLGrammarPoolImpl(MemoryManager* const memMgr);

    ~XMLGrammarPoolImpl();
    //@}

    // -----------------------------------------------------------------------
    /** @name Implementation of Grammar Pool Interface */
    // -----------------------------------------------------------------------
    //@{
   
    /**
      * cacheGrammar
      *
      * Provide the grammar pool with an opportunity
      * to cache the given grammar.  If the pool does not choose to do so,
      * it should return false; otherwise, it should return true, so that
      * the caller knows whether the grammar has been adopted.
      *
      * @param gramToCache: the Grammar to be cached in the grammar pool
      * @return true if the grammar pool has elected to cache the grammar (in which case
      * it is assumed to have adopted it); false if it does not cache it
	  *
      */
    virtual bool           cacheGrammar(Grammar* const               gramToCache);
    

    /**
      * retrieveGrammar
      *
      * @param gramDesc: the Grammar Description used to search for grammar
	  *                  cached in the grammar pool
	  *
      */
    virtual Grammar*       retrieveGrammar(XMLGrammarDescription* const gramDesc);
    
        
    /**
      * orphanGrammar
      *
	  * grammar removed from the grammar pool and owned by the caller
      *
      * @param nameSpaceKey: Key used to search for grammar in the grammar pool
	  *
      */
    virtual Grammar*       orphanGrammar(const XMLCh* const nameSpaceKey);  


    /**
     * Get an enumeration of the cached Grammars in the Grammar pool
     *
     * @return enumeration of the cached Grammars in Grammar pool
     */
    virtual RefHashTableOfEnumerator<Grammar> getGrammarEnumerator() const;

    /**
      * clear
      *
	  * all grammars are removed from the grammar pool and deleted.
      * @return true if the grammar pool was cleared. false if it did not.
      */
    virtual bool           clear();
        
    /**
      * lockPool
      *
	  * When this method is called by the application, the 
      * grammar pool should stop adding new grammars to the cache.
      */
    virtual void           lockPool();
    
    /**
      * unlockPool
      *
	  * After this method has been called, the grammar pool implementation
      * should return to its default behaviour when cacheGrammars(...) is called.
      *
      * For PSVI support any previous XSModel that was produced will be deleted.
      */
    virtual void           unlockPool();

    //@}

    // -----------------------------------------------------------------------
    /** @name  Implementation of Factory interface */
    // -----------------------------------------------------------------------
    //@{

    /**
      * createDTDGrammar
      *
      */
    virtual DTDGrammar*            createDTDGrammar();

    /**
      * createSchemaGrammar
      *
      */
    virtual SchemaGrammar*         createSchemaGrammar();
                    
    /**
      * createDTDDescription
      *
      */	
    virtual XMLDTDDescription*     createDTDDescription(const XMLCh* const rootName);
    /**
      * createSchemaDescription
      *
      */		
    virtual XMLSchemaDescription*  createSchemaDescription(const XMLCh* const targetNamespace);
    //@}
	
    // -----------------------------------------------------------------------
    /** @name  schema component model support */
    // -----------------------------------------------------------------------                                                        
    //@{

    /***
      * Return an XSModel derived from the components of all SchemaGrammars
      * in the grammar pool.  If the pool is locked, this should
      * be a thread-safe operation.  It should return null if and only if
      * the pool is empty.
      *
      * Calling getXSModel() on an unlocked grammar pool may result in the
      * creation of a new XSModel with the old XSModel being deleted.  The
      * function will return a different address for the XSModel if it has
      * changed.
      *
      * In this implementation, when the pool is not locked a new XSModel will be
      * computed each this time the pool is called if the pool has changed (and the
      * previous one will be destroyed at that time).  When the lockPool()
      * method is called, an XSModel will be generated and returned whenever this method is called
      * while the pool is in the locked state.  This will be destroyed if the unlockPool()
      * operation is called.  The XSModel will not be serialized,
      * but will be recreated if a deserialized pool is in the 
      * locked state.
      */
    virtual XSModel *getXSModel();
	
    // @}
    // -----------------------------------------------------------------------
    /** @name  Getter */
    // -----------------------------------------------------------------------                                                        
    //@{

    /**
      * Return an XMLStringPool for use by validation routines.  
      * Implementations should not create a string pool on each call to this
      * method, but should maintain one string pool for all grammars
      * for which this pool is responsible.
      */
    virtual XMLStringPool *getURIStringPool();

    // @}

    // -----------------------------------------------------------------------
    // serialization and deserialization support
    // -----------------------------------------------------------------------

    /***
      *
      * Multiple serializations
      *
      *    For multiple serializations, if the same file name is given, then the 
      *    last result will be in the file (overwriting mode), if different file 
      *    names are given, then there are multiple data stores for each serialization.
      *
      * Multiple deserializations
      * 
      *    Not supported
      *
      * Versioning
      *
      *    Only binary data serialized with the current XercesC Version and
      *    SerializationLevel is supported.
      *
      * Clean up
      *
      *    In the event of an exception thrown due to a corrupted data store during 
      *    deserialization, this implementation may not be able to clean up all resources 
      *    allocated, and therefore it is the client application's responsibility to 
      *    clean up those unreleased resources.
      *
      * Coupling of Grammars and StringPool
      *
      *    This implementation assumes that StringPool shall always be 
      *    serialized/deserialized together with the grammars. In the case that such a
      *    coupling is not desired, client application can modify this behaviour by 
      *    either derivate from this imlementation and overwrite the serializeGrammars()
      *    and/or deserializeGrammars() to decouple grammars and string pool, or
      *    Once deserializeGrammars() is done, insert another StringPool through
      *    setStringPool().
      *
      *    Client application shall be aware of the unpredicatable/undefined consequence 
      *    of this decoupling.
      */

    virtual void     serializeGrammars(BinOutputStream* const); 
    virtual void     deserializeGrammars(BinInputStream* const); 

    friend class XObjectComparator;
    friend class XTemplateComparator;

private:

    virtual void    createXSModel();
    // -----------------------------------------------------------------------
    /** name  Unimplemented copy constructor and operator= */
    // -----------------------------------------------------------------------
    //@{
    XMLGrammarPoolImpl(const XMLGrammarPoolImpl& );
    XMLGrammarPoolImpl& operator=(const XMLGrammarPoolImpl& );
    //@}

    // -----------------------------------------------------------------------
    //
    // fGrammarRegistry: 
    //
	//    container
    // fStringPool
    //    grammars need a string pool for URI -> int mappings
    // fSynchronizedStringPool
    //      When the grammar pool is locked, provide a string pool
    //      that can be updated in a thread-safe manner.
    // fLocked
    //      whether the pool has been locked
    //
    // -----------------------------------------------------------------------
    RefHashTableOf<Grammar>*            fGrammarRegistry; 
    XMLStringPool*                      fStringPool;
    XMLSynchronizedStringPool*          fSynchronizedStringPool;
    XSModel*                            fXSModel;
    bool                                fLocked;
    bool                                fXSModelIsValid;
};

XERCES_CPP_NAMESPACE_END

#endif
