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
 * $Log: LocalFileInputSource.hpp,v $
 * Revision 1.5  2004/01/29 11:46:29  cargilld
 * Code cleanup changes to get rid of various compiler diagnostic messages.
 *
 * Revision 1.4  2003/12/01 23:23:25  neilg
 * fix for bug 25118; thanks to Jeroen Witmond
 *
 * Revision 1.3  2003/05/16 21:36:55  knoaman
 * Memory manager implementation: Modify constructors to pass in the memory manager.
 *
 * Revision 1.2  2002/11/04 15:00:21  tng
 * C++ Namespace Support.
 *
 * Revision 1.1.1.1  2002/02/01 22:21:50  peiyongz
 * sane_include
 *
 * Revision 1.7  2001/10/13 04:21:53  jasons
 * This patch resolves bug #2409: undocumented XMLException in LocalFileInputSource::new()
 *
 * Revision 1.6  2000/12/14 18:49:53  tng
 * Fix API document generation warning: "Warning: end of member group without matching begin"
 *
 * Revision 1.5  2000/02/24 20:00:22  abagchi
 * Swat for removing Log from API docs
 *
 * Revision 1.4  2000/02/15 23:59:06  roddey
 * More updated documentation of Framework classes.
 *
 * Revision 1.3  2000/02/15 01:21:30  roddey
 * Some initial documentation improvements. More to come...
 *
 * Revision 1.2  2000/02/06 07:47:46  rahulj
 * Year 2K copyright swat.
 *
 * Revision 1.1  2000/01/12 18:58:06  roddey
 * Putting this back with the correct lower case extension.
 *
 * Revision 1.1  2000/01/12 00:13:26  roddey
 * These were moved from internal/ to framework/, which was something that should have
 * happened long ago. They are really framework type of classes.
 *
 * Revision 1.1.1.1  1999/11/09 01:08:10  twl
 * Initial checkin
 *
 * Revision 1.2  1999/11/08 20:44:43  rahul
 * Swat for adding in Product name and CVS comment log variable.
 *
 */


#if !defined(LOCALFILEINPUTSOURCE_HPP)
#define LOCALFILEINPUTSOURCE_HPP

#include <xercesc/sax/InputSource.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class BinInputStream;

/**
 *  This class is a derivative of the standard InputSource class. It provides
 *  for the parser access to data which is referenced via a local file path,
 *  as apposed to remote file or URL. This is the most efficacious mechanism
 *  by which local files can be parsed, since the parse knows that it refers
 *  to a local file and will make no other attempts to interpret the passed
 *  path.
 *
 *  The path provided can either be a fully qualified path or a relative path.
 *  If relative, it will be completed either relative to a passed base path
 *  or relative to the current working directory of the process.
 *
 *  As with all InputSource derivatives. The primary objective of an input
 *  source is to create an input stream via which the parser can spool in
 *  data from the referenced source.
 */
class XMLPARSER_EXPORT LocalFileInputSource : public InputSource
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------

    /** @name Constructors */
    //@{

    /**
      * A local file input source requires a path to the file to load. This
      * can be provided either as a fully qualified path, a path relative to
      * the current working directly, or a path relative to a provided base
      * path.
      *
      * The completed path will become the system id of this input source.
      * The constructors don't take any public id for local files, but you
      * still set them via the parent class' setPublicId() method of course.
      *
      * This constructor takes an explicit base path and a possibly relative
      * path. If the relative path is seen to be fully qualified, it is used
      * as is. Otherwise, it is made relative to the passed base path.
      *
      * @param  basePath    The base path from which the passed relative path
      *                     will be based, if the relative part is indeed
      *                     relative.
      *
      * @param  relativePath    The relative part of the path. It can actually
      *                         be fully qualified, in which case it is taken
      *                         as is.
      *
      * @param  manager    Pointer to the memory manager to be used to
      *                    allocate objects.
      *
      * @exception XMLException If the path is relative and doesn't properly
      *            resolve to a file.
      */
    LocalFileInputSource
    (
        const   XMLCh* const   basePath
        , const XMLCh* const   relativePath
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    /**
      * This constructor takes a single parameter which is the fully qualified
      * or relative path. If it is fully qualified, it is taken as is. If it is
      * relative, then it is completed relative to the current working directory
      * (or the equivalent on the local host machine.)
      *
      * The completed path will become the system id of this input source.
      * The constructors don't take any public id for local files, but you
      * still set them via the parent class' setPublicId() method of course.
      *
      * @param  filePath    The relative or fully qualified path.
      *
      * @param  manager     Pointer to the memory manager to be used to
      *                     allocate objects.
      *
      * @exception XMLException If the path is relative and doesn't properly
      *            resolve to a file.
      */
    LocalFileInputSource
    (
        const   XMLCh* const   filePath
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    //@}

    /** @name Destructor */
    //@{
    ~LocalFileInputSource();
    //@}


    // -----------------------------------------------------------------------
    //  Virtual input source interface
    // -----------------------------------------------------------------------

    /** @name Virtual methods */
    //@{

    /**
    * This method will return a binary input stream derivative that will
    * parse from the local file indicatedby the system id.
    *
    * @return A dynamically allocated binary input stream derivative that
    *         can parse from the file indicated by the system id.
    */
    virtual BinInputStream* makeStream() const;

    //@}
private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    LocalFileInputSource(const LocalFileInputSource&);
    LocalFileInputSource& operator=(const LocalFileInputSource&);

};

XERCES_CPP_NAMESPACE_END

#endif
