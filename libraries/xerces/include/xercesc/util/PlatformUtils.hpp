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
 * $Id: PlatformUtils.hpp 180016 2005-06-04 19:49:30Z jberry $
 */


#if !defined(PLATFORMUTILS_HPP)
#define PLATFORMUTILS_HPP

#include <xercesc/util/XMLException.hpp>
#include <xercesc/util/PanicHandler.hpp>

XERCES_CPP_NAMESPACE_BEGIN

class XMLMsgLoader;
class XMLNetAccessor;
class XMLTransService;
class MemoryManager;
class XMLMutex;

//
//  For internal use only
//
//  This class provides a simple abstract API via which lazily evaluated
//  data can be cleaned up.
//
class XMLUTIL_EXPORT XMLDeleter
{
public :
    virtual ~XMLDeleter();

protected :
    XMLDeleter();

private :
    XMLDeleter(const XMLDeleter&);
    XMLDeleter& operator=(const XMLDeleter&);
};


/**
  * Utilities that must be implemented in a platform-specific way.
  *
  * This class contains methods that must be implemented in a platform
  * specific manner. The actual implementations of these methods are
  * available in the per-platform files indide <code>src/util/Platforms
  * </code>.
  */
class XMLUTIL_EXPORT XMLPlatformUtils
{
public :

    /** @name Public Static Data */
    //@{

    /** The network accessor
      *
      * This is provided by the per-platform driver, so each platform can
      * choose what actual implementation it wants to use. The object must
      * be dynamically allocated.
      *
      * <i>Note that you may optionally, if your platform driver does not
      * install a network accessor, set it manually from your client code
      * after calling Initialize(). This works because this object is
      * not required during initialization, and only comes into play during
      * actual XML parsing.</i>
      */
    static XMLNetAccessor*      fgNetAccessor;

    /** The transcoding service.
      *
      * This is provided by the per platform driver, so each platform can
      * choose what implemenation it wants to use. When the platform
      * independent initialization code needs to get a transcoding service
      * object, it will call <code>makeTransService()</code> to ask the
      * per-platform code to create one. Only one transcoding service
      * object is reqeusted per-process, so it is shared and synchronized
      * among parser instances within that process.
      */
    static XMLTransService*     fgTransService;

    /** The Panic Handler
      *
      *   This is the application provided panic handler. 
      */
    static PanicHandler*        fgUserPanicHandler;
    
    /** The Panic Handler
      *
      *   This is the default panic handler. 
      */    
    static PanicHandler*        fgDefaultPanicHandler;

    /** The configurable memory manager
      *
      *   This is the pluggable memory manager. If it is not provided by an
      *   application, a default implementation is used.
      */
    static MemoryManager*       fgMemoryManager;
    
    /** The array-allocating memory manager
      *
      *   This memory manager always allocates memory by calling the
      *   global new[] operator. It may be used to allocate memory
      *   where such memory needs to be deletable by calling delete [].
      *   Since this allocator is always guaranteed to do the same thing
      *   there is no reason, nor facility, to override it.
      */
    static MemoryManager*       fgArrayMemoryManager;

    static XMLMutex*            fgAtomicMutex;
    
    //@}


    /** @name Initialization amd Panic methods */
    //@{

    /** Perform per-process parser initialization
      *
      * Initialization <b>must</b> be called first in any client code.
      *
      * The locale is set iff the Initialize() is invoked for the very first time,
      * to ensure that each and every message loaders, in the process space, share
      * the same locale.
      *
      * All subsequent invocations of Initialize(), with a different locale, have
      * no effect on the message loaders, either instantiated, or to be instantiated.
      *
      * To set to a different locale, client application needs to Terminate() (or
      * multiple Terminate() in the case where multiple Initialize() have been invoked
      * before), followed by Initialize(new_locale).
      *
      * The default locale is "en_US".
      *
      * nlsHome: user specified location where MsgLoader retrieves error message files.
      *          the discussion above with regard to locale, applies to this nlsHome
      *          as well.
      *
      * panicHandler: application's panic handler, application owns this handler.
      *               Application shall make sure that the plugged panic handler persists 
      *               through the call to XMLPlatformUtils::terminate().       
      *
      * memoryManager: plugged-in memory manager which is owned by user
      *                applications. Applications must make sure that the
      *                plugged-in memory manager persist through the call to
      *                XMLPlatformUtils::terminate()
      */
    static void Initialize(const char*          const locale = XMLUni::fgXercescDefaultLocale
                         , const char*          const nlsHome = 0
                         ,       PanicHandler*  const panicHandler = 0
                         ,       MemoryManager* const memoryManager = 0
                         ,       bool                 toInitStatics = false);

    /** Perform per-process parser termination
      *
      * The termination call is currently optional, to aid those dynamically
      * loading the parser to clean up before exit, or to avoid spurious
      * reports from leak detectors.
      */
    static void Terminate();

    /** The panic mechanism.
      *
      * If, during initialization, we cannot even get far enough along
      * to get transcoding up or get message loading working, we call
      * this method.</p>
      *
      * Each platform can implement it however they want. This method will
      * delegate the panic handling to a user specified panic handler or
      * in the absence of it, the default panic handler.
      *
      * In case the default panic handler does not support a particular
      * platform, the platform specific panic hanlding shall be implemented
      * here </p>.
      * 
      * @param reason The enumeration that defines the cause of the failure
      */
    static void panic
    (
        const   PanicHandler::PanicReasons    reason
    );
    
    //@}

    /** @name File Methods */
    //@{

    /** Get the current file position
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to deterine the current position within
      * the passed file.
      *
      * Since the file API provided here only reads, if the host platform
      * supports separate read/write positions, only the read position is
      * of any interest, and hence should be the one returned.
      *
      * @param theFile The file handle
      * @param manager The MemoryManager to use to allocate objects
      */
    static unsigned int curFilePos(FileHandle theFile
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    /** Closes the file handle
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to close the passed file handle, and to
      * destroy the passed file handle and any allocated data or system
      * resources it contains.
      *
      * @param theFile The file handle to close
      * @param manager The MemoryManager to use to allocate objects
      */
    static void closeFile(FileHandle theFile
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    /** Returns the file size
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to determine the current size of the file
      * represented by the passed handle.
      *
      * @param theFile The file handle whose size you want
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns the size of the file in bytes
      */
    static unsigned int fileSize(FileHandle theFile
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    /** Opens the file
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to open passed file. If it fails, a
      * null handle pointer should be returned.
      *
      * @param fileName The string containing the name of the file
      * @param manager The MemoryManager to use to allocate objects
      * @return The file handle of the opened file
      */
    static FileHandle openFile(const char* const fileName
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    /** Opens a named file
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to open the passed file. If it fails, a
      * null handle pointer should be returned.
      *
      * @param fileName The string containing the name of the file
      * @param manager The MemoryManager to use to allocate objects
      * @return The file handle of the opened file
      */
    static FileHandle openFile(const XMLCh* const fileName
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    /** Open a named file to write
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to open passed file. If it fails, a
      * null handle pointer should be returned.
      *
      * @param fileName The string containing the name of the file
      * @param manager The MemoryManager to use to allocate objects
      * @return The file handle of the opened file
      */
    static FileHandle openFileToWrite(const char* const fileName
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    /** Open a named file to write
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to open the passed file. If it fails, a
      * null handle pointer should be returned.
      *
      * @param fileName The string containing the name of the file
      * @param manager The MemoryManager to use to allocate objects
      * @return The file handle of the opened file
      */
    static FileHandle openFileToWrite(const XMLCh* const fileName
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    /** Opens the standard input as a file
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to open a handle to the standard input.
      * It should be a copy of the standard input handle, since it will
      * be closed later!
      *
      * @param manager The MemoryManager to use to allocate objects
      * @return The file handle of the standard input stream
      */
    static FileHandle openStdInHandle(MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    /** Reads the file buffer
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to read up to 'toRead' bytes of data from
      * the passed file, and return those bytes in the 'toFill' buffer. It
      * is not an error not to read the requested number of bytes. When the
      * end of file is reached, zero should be returned.
      *
      * @param theFile The file handle to be read from.
      * @param toRead The maximum number of byte to read from the current
      * position
      * @param toFill The byte buffer to fill
      * @param manager The MemoryManager to use to allocate objects
      *
      * @return Returns the number of bytes read from the stream or file
      */
    static unsigned int readFileBuffer
    (
                FileHandle      theFile
        , const unsigned int    toRead
        ,       XMLByte* const  toFill
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager
    );

    /** Writes the buffer to the file
      *
      * This must be implemented by the per-platform driver, which should
      * use local file services to write up to 'toWrite' bytes of data to
      * the passed file. Unless exception raised by local file services,
      * 'toWrite' bytes of data is to be written to the passed file.
      *
      * @param theFile The file handle to be written to.
      * @param toWrite The maximum number of byte to write from the current
      * position
      * @param toFlush The byte buffer to flush
      * @param manager The MemoryManager to use to allocate objects
      * @return void
      */
    static void writeBufferToFile
    (
          FileHandle     const  theFile
        , long                  toWrite
        , const XMLByte* const  toFlush
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager
    );

    /** Resets the file handle
      *
      * This must be implemented by the per-platform driver which will use
      * local file services to reset the file position to the start of the
      * the file.
      *
      * @param theFile The file handle that you want to reset
      * @param manager The MemoryManager to use to allocate objects
      */
    static void resetFile(FileHandle theFile
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    //@}


    /** @name File System Methods */
    //@{
    /** Gets the full path from a relative path
      *
      * This must be implemented by the per-platform driver. It should
      * complete a relative path using the 'current directory', or whatever
      * the local equivalent of a current directory is. If the passed
      * source path is actually fully qualified, then a straight copy of it
      * will be returned.
      *
      * @param srcPath The path of the file for which you want the full path
      *
      * @param manager Pointer to the memory manager to be used to
      *                allocate objects.
      *
      * @return Returns the fully qualified path of the file name including
      *         the file name. This is dyanmically allocated and must be
      *         deleted  by the caller when its no longer needed! The memory
      *         returned will beallocated using the static memory manager, if
      *         user do not supply a memory manager. Users then need to make
      *         sure to use either the default or user specific memory manager
      *         to deallocate the memory.
      */
    static XMLCh* getFullPath
    (
        const XMLCh* const srcPath
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Gets the current working directory 
      *
      * This must be implemented by the per-platform driver. It returns 
      * the current working directory is. 
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns the current working directory. 
      *         This is dyanmically allocated and must be deleted
      *         by the caller when its no longer needed! The memory returned
      *         will be allocated using the static memory manager, if users
      *         do not supply a memory manager. Users then need to make sure
      *         to use either the default or user specific memory manager to
      *         deallocate the memory.
      */
    static XMLCh* getCurrentDirectory
    (
        MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );

    /** Check if a charater is a slash
      *
      * This must be implemented by the per-platform driver. 
      *
      * @param c the character to be examined
      *
      * @return true  if the character examined is a slash
      *         false otherwise
      */
    static inline bool isAnySlash(XMLCh c);
    
    /** Remove occurences of the pair of dot slash 
      *
      * To remove the sequence, dot slash if it is part of the sequence,
      * slash dot slash.
      *
      * @param srcPath The path for which you want to remove the dot slash sequence.
      * @param manager The MemoryManager to use to allocate objects
      * @return 
      */
    static void   removeDotSlash(XMLCh* const srcPath
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /** Remove occurences of the dot dot slash 
      *
      * To remove the sequence, slash dot dot slash and its preceding path segment
      * if and only if the preceding path segment is not slash dot dot slash.
      *
      * @param srcPath The path for which you want to remove the slash dot
      *        dot slash sequence and its preceding path segment.
      * @param manager The MemoryManager to use to allocate objects
      * @return 
      */
    static void   removeDotDotSlash(XMLCh* const srcPath
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager);

    /** Determines if a path is relative or absolute
      *
      * This must be implemented by the per-platform driver, which should
      * determine whether the passed path is relative or not. The concept
      * of relative and absolute might be... well relative on different
      * platforms. But, as long as the determination is made consistently
      * and in coordination with the weavePaths() method, it should work
      * for any platform.
      *
      * @param toCheck The file name which you want to check
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns true if the filename appears to be relative
      */
    static bool isRelative(const XMLCh* const toCheck
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
        );

    /** Utility to join two paths
      *
      * This must be implemented by the per-platform driver, and should
      * weave the relative path part together with the base part and return
      * a new path that represents this combination.
      *
      * If the relative part turns out to be fully qualified, it will be
      * returned as is. If it is not, then it will be woven onto the
      * passed base path, by removing one path component for each leading
      * "../" (or whatever is the equivalent in the local system) in the
      * relative path.
      *
      * @param basePath The string containing the base path
      * @param relativePath The string containing the relative path
      * @param manager The MemoryManager to use to allocate objects
      * @return Returns a string containing the 'woven' path. It should
      * be dynamically allocated and becomes the responsibility of the
      * caller to delete.
      */
    static XMLCh* weavePaths
    (
        const   XMLCh* const    basePath
        , const XMLCh* const    relativePath
        , MemoryManager* const manager = XMLPlatformUtils::fgMemoryManager
    );
    //@}

    /** @name Timing Methods */
    //@{

    /** Gets the system time in milliseconds
      *
      * This must be implemented by the per-platform driver, which should
      * use local services to return the current value of a running
      * millisecond timer. Note that the value returned is only as accurate
      * as the millisecond time of the underyling host system.
      *
      * @return Returns the system time as an unsigned long
      */
    static unsigned long getCurrentMillis();
    //@}

    /** @name Mutex Methods */
    //@{

    /** Closes a mutex handle
      *
      * Each per-platform driver must implement this. Only it knows what
      * the actual content of the passed mutex handle is.
      *
      * @param mtxHandle The mutex handle that you want to close
      */
    static void closeMutex(void* const mtxHandle);

    /** Locks a mutex handle
      *
      * Each per-platform driver must implement this. Only it knows what
      * the actual content of the passed mutex handle is.
      *
      * @param mtxHandle The mutex handle that you want to lock
      */
    static void lockMutex(void* const mtxHandle);

    /** Make a new mutex
      *
      * Each per-platform driver must implement this. Only it knows what
      * the actual content of the passed mutex handle is. The returned
      * handle pointer will be eventually passed to closeMutex() which is
      * also implemented by the platform driver.
      *
      * @param manager The MemoryManager to use to allocate objects
      */
    static void* makeMutex(MemoryManager* manager = XMLPlatformUtils::fgMemoryManager);

    /** Unlocks a mutex
      *
      * Each per-platform driver must implement this. Only it knows what
      * the actual content of the passed mutex handle is.
      *
      * Note that, since the underlying system synchronization services
      * are used, Xerces cannot guarantee that lock/unlock operations are
      * correctly enforced on a per-thread basis or that incorrect nesting
      * of lock/unlock operations will be caught.
      *
      * @param mtxHandle The mutex handle that you want to unlock
      */
    static void unlockMutex(void* const mtxHandle);

    //@}


    /** @name External Message Support */
    //@{

    /** Loads the message set from among the available domains
      *
      * The returned object must be dynamically allocated and the caller
      * becomes responsible for cleaning it up.
      *
      * @param msgDomain The message domain which you want to load
      */
    static XMLMsgLoader* loadMsgSet(const XMLCh* const msgDomain);

    //@}

    /** @name Miscellaneous synchronization methods */
    //@{

    /** Conditionally updates or returns a single word variable atomically
      *
      * This must be implemented by the per-platform driver. The
      * compareAndSwap subroutine performs an atomic operation which
      * compares the contents of a single word variable with a stored old
      * value. If the values are equal, a new value is stored in the single
      * word variable and TRUE is returned; otherwise, the old value is set
      * to the current value of the single word variable and FALSE is
      * returned.
      *
      * The compareAndSwap subroutine is useful when a word value must be
      * updated only if it has not been changed since it was last read.
      *
      * Note: The word containing the single word variable must be aligned
      * on a full word boundary.
      *
      * @param toFill Specifies the address of the single word variable
      * @param newValue Specifies the new value to be conditionally assigned
      * to the single word variable.
      * @param toCompare Specifies the address of the old value to be checked
      * against (and conditionally updated with) the value of the single word
      * variable.
      *
      * @return Returns the new value assigned to the single word variable
      */
    static void* compareAndSwap
    (
                void**      toFill
        , const void* const newValue
        , const void* const toCompare
    );

    //@}


    /** @name Atomic Increment and Decrement */
    //@{

    /** Increments a single word variable atomically.
      *
      * This must be implemented by the per-platform driver. The
      * atomicIncrement subroutine increments one word in a single atomic
      * operation. This operation is useful when a counter variable is shared
      * between several threads or processes. When updating such a counter
      * variable, it is important to make sure that the fetch, update, and
      * store operations occur atomically (are not interruptible).
      *
      * @param location Specifies the address of the word variable to be
      * incremented.
      *
      * @return The function return value is positive if the result of the
      * operation was positive. Zero if the result of the operation was zero.
      * Negative if the result of the operation was negative. Except for the
      * zero case, the value returned may differ from the actual result of
      * the operation - only the sign and zero/nonzero state is guaranteed
      * to be correct.
      */
    static int atomicIncrement(int& location);

    /** Decrements a single word variable atomically.
      *
      * This must be implemented by the per-platform driver. The
      * atomicDecrement subroutine increments one word in a single atomic
      * operation. This operation is useful when a counter variable is shared
      * between several threads or processes. When updating such a counter
      * variable, it is important to make sure that the fetch, update, and
      * store operations occur atomically (are not interruptible).
      *
      * @param location Specifies the address of the word variable to be
      * decremented.
      *
      * @return The function return value is positive if the result of the
      * operation was positive. Zero if the result of the operation was zero.
      * Negative if the result of the operation was negative. Except for the
      * zero case, the value returned may differ from the actual result of the
      * operation - only the sign and zero/nonzero state is guaranteed to be
      * correct.
      */
    static int atomicDecrement(int& location);

    //@}

    /** @name NEL Character Handling  */
    //@{
	/**
      * This function enables the recognition of NEL(0x85) char and LSEP (0x2028) as newline chars
      * which is disabled by default.
      * It is only called once per process. Once it is set, any subsequent calls
      * will result in exception being thrown.
      *
      * Note: 1. Turning this option on will make the parser non compliant to XML 1.0.
      *       2. This option has no effect to document conforming to XML 1.1 compliant,
      *          which always recognize these two chars (0x85 and 0x2028) as newline characters.
      *
      */
    static void recognizeNEL(bool state
        , MemoryManager* const manager  = XMLPlatformUtils::fgMemoryManager);

    /**
      * Return the value of fgNEL flag.
      */
    static bool isNELRecognized();
    //@}

    /** @name Strict IANA Encoding Checking */
    //@{
	/**
      * This function enables/disables strict IANA encoding names checking.
      *
      * The strict checking is disabled by default.
      *
      * @param state If true, a strict IANA encoding name check is performed,
      *              otherwise, no checking.
      *
      */
    static void strictIANAEncoding(const bool state);

    /**
      * Returns whether a strict IANA encoding name check is enabled or
      * disabled.
      */
    static bool isStrictIANAEncoding();
    //@}
		
    /**
      * Aligns the specified pointer per platform block allocation
	  * requirements.
	  *
	  *	The results of this function may be altered by defining
	  * XML_PLATFORM_NEW_BLOCK_ALIGNMENT.
	  */
	static inline size_t alignPointerForNewBlockAllocation(size_t ptrSize);

private :
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    XMLPlatformUtils();

    /** @name Private static methods */
    //@{

    /** Loads a message set from the available domains
      *
      * @param msgDomain The message domain containing the message to be
      * loaded
      */
    static XMLMsgLoader* loadAMsgSet(const XMLCh* const msgDomain);

    /** Creates a net accessor object.
      *
      * Each per-platform driver must implement this method. However,
      * having a Net Accessor is optional and this method can return a
      * null pointer if remote access via HTTP and FTP URLs is not required.
      *
      * @return An object derived from XMLNetAccessor. It must be dynamically
      *         allocated, since it will be deleted later.
      */
    static XMLNetAccessor* makeNetAccessor();

    /** Creates a Transoding service
      *
      * Each per-platform driver must implement this method and return some
      * derivative of the XMLTransService class. This object serves as the
      * transcoder factory for this process. The object must be dynamically
      * allocated and the caller is responsible for cleaning it up.
      *
      * @return A dynamically allocated object of some class derived from
      *         the XMLTransService class.
      */
    static XMLTransService* makeTransService();

    /** Does initialization for a particular platform
      *
      * Each per-platform driver must implement this to do any low level
      * system initialization required. It <b>cannot</b> use any XML
      * parser or utilities services!
      */
    static void platformInit();

    /** Does termination for a particular platform
      *
      * Each per-platform driver must implement this to do any low level
      * system resource cleanup required. It <b>cannot</b> use any XML
      * parser or utilities services!
      */
    static void platformTerm();

    /** Search for sequence, slash dot dot slash
      *
      * @param srcPath the path to search
      *
      * @return   the position of the first occurence of slash dot dot slash
      *            -1 if no such sequence is found
      */
    static int  searchSlashDotDotSlash(XMLCh* const srcPath);

    //@}

    /** @name Private static methods */
    //@{

    /**
      * Indicates whether the memory manager was supplied by the user
      * or not. Users own the memory manager, and if none is supplied,
      * Xerces uses a default one that it owns and is responsible for
      * deleting in Terminate().
      */
    static bool fgMemMgrAdopted;

    //@}
};


MakeXMLException(XMLPlatformUtilsException, XMLUTIL_EXPORT)


// ---------------------------------------------------------------------------
//  XMLPlatformUtils: alignPointerForNewBlockAllocation
// ---------------------------------------------------------------------------
//  Calculate alignment required by platform for a new
//	block allocation. We use this in our custom allocators
//	to ensure that returned blocks are properly aligned.
//  Note that, although this will take a pointer and return the position
//  at which it should be placed for correct alignment, in our code
//  we normally use size_t parameters to discover what the alignment
//  of header blocks should be.  Thus, if this is to be
//  used for the former purpose, to make compilers happy
//  some casting will be necessary - neilg.
//
//  Note: XML_PLATFORM_NEW_BLOCK_ALIGNMENT may be specified on a
//        per-architecture basis to dictate the alignment requirements
//        of the architecture. In the absense of this specification,
//        this routine guesses at the correct alignment value.
//
//        A XML_PLATFORM_NEW_BLOCK_ALIGNMENT value of zero is illegal.
//        If a platform requires absolutely no alignment, a value
//        of 1 should be specified ("align pointers on 1 byte boundaries").
//
inline size_t
XMLPlatformUtils::alignPointerForNewBlockAllocation(size_t ptrSize)
{
	//	Macro XML_PLATFORM_NEW_BLOCK_ALIGNMENT may be defined
	//	as needed to dictate alignment requirements on a
	//	per-architecture basis. In the absense of that we
	//	take an educated guess.
	#ifdef XML_PLATFORM_NEW_BLOCK_ALIGNMENT
		size_t alignment = XML_PLATFORM_NEW_BLOCK_ALIGNMENT;
	#else
		size_t alignment = (sizeof(void*) >= sizeof(double)) ? sizeof(void*) : sizeof(double);
	#endif
	
	//	Calculate current alignment of pointer
	size_t current = ptrSize % alignment;
	
	//	Adjust pointer alignment as needed
	return (current == 0)
		 ? ptrSize
		 : (ptrSize + alignment - current);
}



// ---------------------------------------------------------------------------
//  XMLDeleter: Public Destructor
// ---------------------------------------------------------------------------
inline XMLDeleter::~XMLDeleter()
{
}

// ---------------------------------------------------------------------------
//  XMLDeleter: Hidden constructors and operators
// ---------------------------------------------------------------------------
inline XMLDeleter::XMLDeleter()
{
}

XERCES_CPP_NAMESPACE_END

#endif
