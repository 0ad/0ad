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
 * $Log: ICUMsgLoader.hpp,v $
 * Revision 1.8  2003/12/24 15:24:13  cargilld
 * More updates to memory management so that the static memory manager.
 *
 * Revision 1.7  2003/12/17 03:56:15  neilg
 * add default memory manager parameter to loadMsg method that uses char * parameters
 *
 * Revision 1.6  2003/05/15 18:29:48  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.5  2003/03/07 18:15:44  tng
 * Return a reference instead of void for operator=
 *
 * Revision 1.4  2002/11/04 15:10:40  tng
 * C++ Namespace Support.
 *
 * Revision 1.3  2002/10/10 21:07:55  peiyongz
 * load resource files using environement vars and base name
 *
 * Revision 1.2  2002/09/30 22:20:40  peiyongz
 * Build with ICU MsgLoader
 *
 * Revision 1.1.1.1  2002/02/01 22:22:19  peiyongz
 * sane_include
 *
 * Revision 1.5  2002/01/21 14:52:25  tng
 * [Bug 5847] ICUMsgLoader can't be compiled with gcc 3.0.3 and ICU2.  And also fix the memory leak introduced by Bug 2730 fix.
 *
 * Revision 1.4  2001/11/01 23:39:18  jasons
 * 2001-11-01  Jason E. Stewart  <jason@openinformatics.com>
 *
 * 	* src/util/MsgLoaders/ICU/ICUMsgLoader.hpp (Repository):
 * 	* src/util/MsgLoaders/ICU/ICUMsgLoader.cpp (Repository):
 * 	Updated to compile with ICU-1.8.1
 *
 * Revision 1.3  2000/03/02 19:55:14  roddey
 * This checkin includes many changes done while waiting for the
 * 1.1.0 code to be finished. I can't list them all here, but a list is
 * available elsewhere.
 *
 * Revision 1.2  2000/02/06 07:48:21  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1.1.1  1999/11/09 01:07:24  twl
 * Initial checkin
 *
 * Revision 1.4  1999/11/08 20:45:26  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(ICUMSGLOADER_HPP)
#define ICUMSGLOADER_HPP

#include <xercesc/util/XMLMsgLoader.hpp>
#include "unicode/ures.h"

XERCES_CPP_NAMESPACE_BEGIN

//
//  This is the ICU specific implementation of the XMLMsgLoader interface.
//  This one uses ICU resource bundles to store its messages.
//
class XMLUTIL_EXPORT ICUMsgLoader : public XMLMsgLoader
{
public :
    // -----------------------------------------------------------------------
    //  Public Constructors and Destructor
    // -----------------------------------------------------------------------
    ICUMsgLoader(const XMLCh* const  msgDomain);
    ~ICUMsgLoader();


    // -----------------------------------------------------------------------
    //  Implementation of the virtual message loader API
    // -----------------------------------------------------------------------
    virtual bool loadMsg
    (
        const   XMLMsgLoader::XMLMsgId  msgToLoad
        ,       XMLCh* const            toFill
        , const unsigned int           maxChars
    );

    virtual bool loadMsg
    (
        const   XMLMsgLoader::XMLMsgId  msgToLoad
        ,       XMLCh* const            toFill
        , const unsigned int            maxChars
        , const XMLCh* const            repText1
        , const XMLCh* const            repText2 = 0
        , const XMLCh* const            repText3 = 0
        , const XMLCh* const            repText4 = 0
        , MemoryManager* const          manager  = XMLPlatformUtils::fgMemoryManager
    );

    virtual bool loadMsg
    (
        const   XMLMsgLoader::XMLMsgId  msgToLoad
        ,       XMLCh* const            toFill
        , const unsigned int            maxChars
        , const char* const             repText1
        , const char* const             repText2 = 0
        , const char* const             repText3 = 0
        , const char* const             repText4 = 0
        , MemoryManager * const         manager  = XMLPlatformUtils::fgMemoryManager
    );


private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    ICUMsgLoader();
    ICUMsgLoader(const ICUMsgLoader&);
    ICUMsgLoader& operator=(const ICUMsgLoader&);


    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fLocaleBundle
    //      pointer to the required locale specific resource bundle,
	//           or to the default locale resrouce bundle in case the required
	//              locale specific resource bundle unavailable.
    //
    //  fDomainBundle
    //      pointer to the domain specific resource bundle with in the
	//              required locale specific (or default locale) resource bundle.
    //
    // -----------------------------------------------------------------------
    UResourceBundle*      fLocaleBundle;
    UResourceBundle*      fDomainBundle;
};

XERCES_CPP_NAMESPACE_END

#endif
