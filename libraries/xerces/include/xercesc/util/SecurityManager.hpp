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
 * $Log: SecurityManager.hpp,v $
 * Revision 1.3  2004/01/29 11:48:46  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.2  2003/04/22 12:53:38  neilg
 * change const static member to an enum to make MSVC happy
 *
 * change ENTITY_EXPANSION_LIMIT from a static const data member to an enum
 * 
 * Revision 1.1  2003/04/17 21:58:49  neilg
 * Adding a new property,
 * http://apache.org/xml/properties/security-manager, with
 * appropriate getSecurityManager/setSecurityManager methods on DOM
 * and SAX parsers.  Also adding a new SecurityManager class.
 *
 * The purpose of these modifications is to permit applications a
 * means to have the parser reject documents whose processing would
 * otherwise consume large amounts of system resources.  Malicious
 * use of such documents could be used to launch a denial-of-service
 * attack against a system running the parser.  Initially, the
 * SecurityManager only knows about attacks that can result from
 * exponential entity expansion; this is the only known attack that
 * involves processing a single XML document.  Other, simlar attacks
 * can be launched if arbitrary schemas may be parsed; there already
 * exist means (via use of the EntityResolver interface) by which
 * applications can deny processing of untrusted schemas.  In future,
 * the SecurityManager will be expanded to take these other exploits
 * into account.
 *
 * Initial checkin of SecurityManager
 *
 * $Id: SecurityManager.hpp,v 1.3 2004/01/29 11:48:46 cargilld Exp $
 *
 */

#ifndef SECURITYMANAGER_HPP
#define SECURITYMANAGER_HPP

#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * Allow application to force the parser to behave in a security-conscious
  * way.
  *
  * <p> There are cases in which an XML- or XmL-schema-
  * conformant processor can be presented with documents the
  * processing of which can involve the consumption of
  * prohibitive amounts of system resources.  Applications can
  * attach instances of this class to parsers that they've
  * created, via the
  * http://apache.org/xml/properties/security-manager property.  
  * </p>
  *
  * <p> Defaults will be provided for all known security holes.
  * Setter methods will be provided on this class to ensure that
  * an application can customize each limit as it chooses.
  * Components that are vulnerable to any given hole need to be
  * written to act appropriately when an instance of this class
  * has been set on the calling parser.
  * </p>
  */

class XMLUTIL_EXPORT SecurityManager
{
public:

    enum { ENTITY_EXPANSION_LIMIT = 50000};

    /** @name default Constructors */
    //@{
    /** Default constructor */
    SecurityManager()
        : fEntityExpansionLimit(ENTITY_EXPANSION_LIMIT)
    {        
    }

    /** Destructor */
    virtual ~SecurityManager(){};   
    //@}

    /** @name The Security Manager */
    //@{
   /**
    * An application should call this method when it wishes to specify a particular
    * limit to the number of entity expansions the parser will permit in a
    * particular document.  The default behaviour should allow the parser
    * to validate nearly all XML non-malicious XML documents; if an
    * application knows that it is operating in a domain where entities are
    * uncommon, for instance, it may wish to provide a limit lower than the
    * parser's default.
    *
    * @param newLimit  the new entity expansion limit
    *
    */
    virtual void setEntityExpansionLimit(unsigned int newLimit) 
    {
        fEntityExpansionLimit = newLimit;
    }

   /**
    * Permits the application or a parser component to query the current
    * limit for entity expansions.
    *
    * @return   the current setting of the entity expansion limit
    *
    */
    virtual unsigned int getEntityExpansionLimit() const
    { 
        return fEntityExpansionLimit;
    }
    //@}

protected:
    unsigned int fEntityExpansionLimit;

private:

    /* Unimplemented Constructors and operators */
    /* Copy constructor */
    SecurityManager(const SecurityManager&);
    
    /** Assignment operator */
    SecurityManager& operator=(const SecurityManager&);
};

XERCES_CPP_NAMESPACE_END

#endif
