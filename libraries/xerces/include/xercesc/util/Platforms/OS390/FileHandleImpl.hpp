/*
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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
 * $Id: FileHandleImpl.hpp,v 1.4 2004/01/12 22:00:18 cargilld Exp $
 */

#ifndef FILEHANDLEIMPL_HPP
#define FILEHANDLEIMPL_HPP
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMemory.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class FileHandleImpl : public XMemory
{
  private:
  FILE*      Handle;       // handle from fopen
  XMLByte*   stgBufferPtr; // address of staging buffer
  int        nextByte;     // NAB in staging buffer
  int        openType;     // 0=write, 1=read
  int        lrecl;        // LRECL if openType is write
  bool       recordType;   // true if "type=record"
  MemoryManager* const fMemoryManager;

  public:
      FileHandleImpl(FILE* open_handle, int o_type, bool r_type, int fileLrecl=0, MemoryManager* const manager=XMLPlatformUtils::fgMemoryManager);
 ~FileHandleImpl();
  void  setHandle(FILE* newHandlePtr) { Handle = newHandlePtr; }
  void* getHandle() { return Handle; }
  XMLByte* getStgBufferPtr() { return stgBufferPtr; }
  int   getNextByte() { return nextByte; }
  void  setNextByte(int newNextByte)  { nextByte = newNextByte; }
  int   getOpenType() { return openType; }
  bool  isRecordType() { return recordType; }
  void  setRecordType(bool newType) { recordType = newType; }
  int   getLrecl() { return lrecl; }
  void  setLrecl(int newLrecl)  { lrecl = newLrecl; }
};

// Constants for the openType member
#define _FHI_WRITE 0
#define _FHI_READ 1
// Constant for the typeRecord member
#define _FHI_NOT_TYPE_RECORD 0
#define _FHI_TYPE_RECORD 1

XERCES_CPP_NAMESPACE_END

#endif
