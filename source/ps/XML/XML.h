/*
	XML.h - Xerces wrappers & convenience functions
	
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
#include "lib/nommgr.h"

// temporarily go down to W3 because Xerces (in addition to all its other
// failings) isn't W4-clean.
#if MSC_VERSION
#pragma warning(push, 3)
#endif

#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <xercesc/sax/InputSource.hpp>
#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/util/BinMemInputStream.hpp>

#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/ErrorHandler.hpp>

// for Xeromyces.cpp (moved here so we only have to #undef new and
// revert to W3 once)
// The converter uses SAX2, so it should [theoretically]
// be fairly easy to swap Xerces for something else (if desired)
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>


#if MSC_VERSION
#pragma warning(pop)	// back to W4
#endif

#include "lib/mmgr.h"		// restore malloc/new macros

#include "lib/lib.h"
#include "lib/res/handle.h"
#include "lib/res/file/file.h"
#include "XercesErrorHandler.h"
#include "ps/CStr.h"

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
	FileIOBuf m_pBuffer;
	size_t m_BufferSize;
	
	CVFSInputSource(const CVFSInputSource &);
	CVFSInputSource &operator = (const CVFSInputSource &);
	
public:
	CVFSInputSource():
		m_pBuffer(NULL),
		m_BufferSize(0)
	{}
	
	virtual ~CVFSInputSource();
	
	// Open a VFS path for XML parsing
	// returns 0 if successful, -1 on failure
	int OpenFile(const char *path, uint flags);

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
