/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 1999-2000 The Apache Software Foundation.  All rights
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
 * $Log: XMLReaderFactory.hpp,v $
 * Revision 1.6  2004/01/29 11:46:32  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.5  2003/06/20 18:56:45  peiyongz
 * Stateless Grammar Pool :: Part I
 *
 * Revision 1.4  2003/05/15 18:27:11  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.3  2002/11/04 14:55:45  tng
 * C++ Namespace Support.
 *
 * Revision 1.2  2002/05/07 17:45:52  knoaman
 * SAX2 documentation update.
 *
 * Revision 1.1.1.1  2002/02/01 22:22:09  peiyongz
 * sane_include
 *
 * Revision 1.3  2000/08/30 22:21:37  andyh
 * Unix Build script fixes.  Clean up some UNIX compiler warnings.
 *
 * Revision 1.2  2000/08/07 18:21:27  jpolast
 * change SAX_EXPORT module to SAX2_EXPORT
 *
 * Revision 1.1  2000/08/02 18:02:35  jpolast
 * initial checkin of sax2 implementation
 * submitted by Simon Fell (simon@fell.com)
 * and Joe Polastre (jpolast@apache.org)
 *
 *
 */

#ifndef XMLREADERFACTORY_HPP
#define XMLREADERFACTORY_HPP

#include <xercesc/parsers/SAX2XMLReaderImpl.hpp>
#include <xercesc/sax/SAXException.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class MemoryManager;
class XMLGrammarPool;

/**
  * Creates a SAX2 parser (SAX2XMLReader).
  *
  * <p>Note: The parser object returned by XMLReaderFactory is owned by the
  * calling users, and it's the responsiblity of the users to delete that
  * parser object, once they no longer need it.</p>
  *
  * @see SAX2XMLReader#SAX2XMLReader
  */
class SAX2_EXPORT XMLReaderFactory
{
protected:                // really should be private, but that causes compiler warnings.
	XMLReaderFactory() ;
	~XMLReaderFactory() ;

public:
	static SAX2XMLReader * createXMLReader( 
                                               MemoryManager* const  manager = XMLPlatformUtils::fgMemoryManager
                                             , XMLGrammarPool* const gramPool = 0
                                               ) ;
	static SAX2XMLReader * createXMLReader(const XMLCh* className)  ;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLReaderFactory(const XMLReaderFactory&);
    XMLReaderFactory& operator=(const XMLReaderFactory&);
};


inline SAX2XMLReader * XMLReaderFactory::createXMLReader(MemoryManager* const  manager
                                                       , XMLGrammarPool* const gramPool)
{
	return (SAX2XMLReader*)(new (manager) SAX2XMLReaderImpl(manager, gramPool));
}

inline SAX2XMLReader * XMLReaderFactory::createXMLReader(const XMLCh *)
{	
	throw SAXNotSupportedException();
	// unimplemented
	return 0;
}

XERCES_CPP_NAMESPACE_END

#endif
