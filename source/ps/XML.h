/*
	XMLUtils.h - Xerces wrappers & convenience functions
	
	AUTHOR	:	Simon Brenner <simon@wildfiregames.com>, <simon.brenner@home.se>
	EXAMPLE	:
		Simple usage:
	
		CVFSEntityResolver *entRes=new CVFSEntityResolver(filename);
		parser->setEntityResolver(entRes);
	
		CVFSInputSource src;
		if (src.OpenFile("this/is/a/vfs/path.xml")==0)
			parser->parse(src);
			
		delete entRes;
		
		The input source object should be kept alive as long as the parser is
		using its input stream (i.e. until the parse is complete). The same
		goes for the entity resolver.
*/

#ifndef _XercesVFS_H
#define _XercesVFS_H

// Temporarily undefine new, because the Xerces headers don't like it
#ifdef HAVE_DEBUGALLOC
# undef new
#endif

#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/sax/InputSource.hpp>
#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/util/BinMemInputStream.hpp>

#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/ErrorHandler.hpp>

#ifdef HAVE_DEBUGALLOC
# define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define ZLIB_WINAPI
#include "zlib.h" // for crc32

#include "res/h_mgr.h"
#include "lib.h"
#include "XercesErrorHandler.h"
#include "CStr.h"

XERCES_CPP_NAMESPACE_USE

CStr XMLTranscode(const XMLCh *);
XMLCh *XMLTranscode(const char *);

/*
	CLASS		: CVFSInputSource
	DESCRIPTION	:
		Use instead of LocalFileInputSource to read XML files from VFS
*/
class CVFSInputSource: public InputSource
{
	Handle m_hMem;	// from vfs_load
	void *m_pBuffer;
	size_t m_BufferSize;
	
public:
	CVFSInputSource():
		m_hMem(0),
		m_pBuffer(NULL),
		m_BufferSize(0)
	{}
	
	~CVFSInputSource();
	
	// Open a VFS path for XML parsing
	// returns 0 if successful, -1 on failure
	int OpenFile(const char *path);

	// Allow the use of externally-loaded files
	void OpenBuffer(const char* path, const void* buffer, const size_t buffersize);

	virtual BinInputStream *makeStream() const;
};

class CVFSEntityResolver: public EntityResolver
{
	const char *m_DocName;

public:
	virtual InputSource *resolveEntity(
		const XMLCh *const publicId,
		const XMLCh *const systemId);
	
	inline CVFSEntityResolver(const char *docName):
		m_DocName(docName)
	{}
};

#endif // _XercesVFS_H
