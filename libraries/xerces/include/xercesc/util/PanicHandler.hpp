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
 * $Log: PanicHandler.hpp,v $
 * Revision 1.6  2003/12/24 17:12:21  cargilld
 * Memory management update.
 *
 * Revision 1.5  2003/12/24 15:24:13  cargilld
 * More updates to memory management so that the static memory manager.
 *
 * Revision 1.4  2003/05/22 18:15:16  neilg
 * The PanicHandler interface should not inherit from XMemory.
 * The reason for this is that the default implementation does not
 * allocate memory dynamically and if such an inheritance relation existed,
 * a user would have to be very careful about installing a memory
 * handler on their own PanicHandler before handing it to the
 * XMLPlatformUtils::Initialize() method, since otherwise
 * the (uninitialized) XMLPlatformUtils::fgMemoryManager would be used
 * upon construction of their PanicHandler implementation.
 *
 * Revision 1.3  2003/05/15 19:04:35  knoaman
 * Partial implementation of the configurable memory manager.
 *
 * Revision 1.2  2003/03/10 16:05:11  peiyongz
 * assignment operator
 *
 * Revision 1.1  2003/03/09 17:06:16  peiyongz
 * PanicHandler
 *
 * $Id: PanicHandler.hpp,v 1.6 2003/12/24 17:12:21 cargilld Exp $
 *
 */

#ifndef PANICHANDLER_HPP
#define PANICHANDLER_HPP

#include <xercesc/util/XMemory.hpp>

XERCES_CPP_NAMESPACE_BEGIN

/**
  * Receive notification of panic.
  *
  * <p>This is the interface, through which the Xercesc reports
  *    a panic to the application. 
  * </p>
  *
  * <p>Application may implement this interface, instantiate an
  *    object of the derivative, and plug it to Xercesc in the
  *    invocation to XMLPlatformUtils::Initialize(), if it prefers 
  *    to handling panic itself rather than Xercesc doing it.
  * </p>
  *
  */

class XMLUTIL_EXPORT PanicHandler
{
public:

    /** @name Public Types */
    //@{
    enum PanicReasons
    {
          Panic_NoTransService
        , Panic_NoDefTranscoder
        , Panic_CantFindLib
        , Panic_UnknownMsgDomain
        , Panic_CantLoadMsgDomain
        , Panic_SynchronizationErr
        , Panic_SystemInit

        , PanicReasons_Count
    };
    //@}

protected:

    /** @name hidden Constructors */
    //@{
    /** Default constructor */
    PanicHandler(){};

public:

    /** Destructor */
    virtual ~PanicHandler(){};   
    //@}

    /** @name The virtual panic handler interface */
    //@{
   /**
    * Receive notification of panic
    *
    * This method is called when an unrecoverable error has occurred in the Xerces library.  
    *
    * This method must not return normally, otherwise, the results are undefined. 
    * 
    * Ways of handling this call could include throwing an exception or exiting the process.
    *
    * Once this method has been called, the results of calling any other Xerces API, 
    * or using any existing Xerces objects are undefined.    
    *
    * @param reason The reason of panic
    *
    */
    virtual void panic(const PanicHandler::PanicReasons reason) = 0;
    //@}

    static const char* getPanicReasonString(const PanicHandler::PanicReasons reason);
    
private:

    /* Unimplemented Constructors and operators */
    /* Copy constructor */
    PanicHandler(const PanicHandler&);
    
    /** Assignment operator */
    PanicHandler& operator=(const PanicHandler&);
};

XERCES_CPP_NAMESPACE_END

#endif
