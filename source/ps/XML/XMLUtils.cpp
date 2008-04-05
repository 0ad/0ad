#include "precompiled.h"

#include "XML.h"
#include "ps/Filesystem.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"

#define LOG_CATEGORY "xml"

/*
// but static Xerces => tons of warnings due to missing debug info,
// and warnings about invalid pointers (conflicting CRT heaps?) in parser => allow for now
#ifndef XERCES_STATIC_LIB
#error "need to define XERCES_STATIC_LIB in project options (so that Xerces uses the same CRT as the other libs)"
#endif
*/

#if MSC_VERSION
# ifdef XERCES_STATIC_LIB
#  ifndef NDEBUG
#   pragma comment(lib, "xerces-c_2D-static.lib")
#  else
#   pragma comment(lib, "xerces-c_2D-static.lib")
#  endif	// NDEBUG
# else		// XERCES_STATIC_LIB
#  ifndef NDEBUG
#   pragma comment(lib, "xerces-c_2D.lib")
#  else
#   pragma comment(lib, "xerces-c_2.lib")
#  endif	// NDEBUG
# endif		// XERCES_STATIC_LIB
#endif		// MSC_VERSION

XERCES_CPP_NAMESPACE_USE

CStr XMLTranscode(const XMLCh* xmltext)
{
	char* str=XMLString::transcode((const XMLCh *)xmltext);
	CStr result(str);
	XMLString::release(&str);
	return result;
}

XMLCh *XMLTranscode(const char *str)
{
	return XMLString::transcode(str);
}

int CVFSInputSource::OpenFile(const VfsPath& path)
{
	LibError ret = g_VFS->LoadFile(path, m_pBuffer, m_BufferSize);
	if(ret != INFO::OK)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CVFSInputSource: file %s couldn't be loaded (LoadFile: %d)", path.string().c_str(), ret);
		return -1;
	}

	XMLCh *sysId=XMLString::transcode(path.string().c_str());
	setSystemId(sysId);
	XMLString::release(&sysId);
	
	return 0;
}

CVFSInputSource::~CVFSInputSource()
{
}

BinInputStream *CVFSInputSource::makeStream() const
{
	if(!m_pBuffer)
		return 0;

	return new BinMemInputStream((XMLByte *)m_pBuffer.get(), (unsigned int)m_BufferSize, BinMemInputStream::BufOpt_Reference);
}

#define IS_PATH_SEP(_chr) (_chr == '/' || _chr == '\\')

// Return a pointer to the last path separator preceding *end, while not
// going further back than *beginning
const char *prevpathcomp(const char *end, const char *beginning)
{
	do
		end--;
	while (end > beginning && !IS_PATH_SEP(*end));
	return end;
}

InputSource *CVFSEntityResolver::resolveEntity(const XMLCh *const UNUSED(publicId),
	const XMLCh *const systemId)
{
	CVFSInputSource *ret=new CVFSInputSource();
	char *path=XMLString::transcode(systemId);
	char *orgpath=path;
	
	char abspath[PATH_MAX];
	const char *end=strchr(m_DocName, '\0');

	if (IS_PATH_SEP(*path))
		path++;
	else
	{
		// We know that we have a relative path here:
		// - Remove the file name
		// - If we have a ../ components - remove them and remove one component
		//   off the end of the document path for each ../ component
		// - prefix of document path + suffix of input path => the VFS path

		// Remove the file name
		end=prevpathcomp(end, m_DocName);
	
		// Remove one path component for each opening ../ (or ..\)
		// Note that this loop will stop when all path components from the
		// document name have been stripped - the resulting path will be invalid, but
		// so was the input path.
		// Also note that this will not handle ../ path components in the middle of
		// the input path.
		while (strncmp(path, "..", 2) == 0 && IS_PATH_SEP(path[2]) && end > m_DocName)
		{
			end=prevpathcomp(end, m_DocName);
			path += 3;
		}

		// include one slash from prefix
		end++;

		const ptrdiff_t prefixlen=end-m_DocName;
		
		cpu_memcpy(abspath, m_DocName, prefixlen);
		strncpy(abspath+prefixlen, path, PATH_MAX-prefixlen);
		// strncpy might not have terminated, if path was too long
		abspath[PATH_MAX-1]=0;
		
		path=abspath;
	}

	// janwas: removed for less spew
//	LOG(CLogger::Normal,  LOG_CATEGORY, "EntityResolver: path \"%s\" translated to \"%s\"", orgpath, path);


	char *pos=path;		
	if ((pos=strchr(pos, '\\')) != NULL)
	{
		LOG(CLogger::Warning, LOG_CATEGORY, "While resolving XML entities for %s: path %s [%s] contains non-portable path separator \\", m_DocName, orgpath, path);
		do
			*pos='/';
		while ((pos=strchr(pos+1, '\\')) != NULL);
	}

	if (ret->OpenFile(path)!=0)
	{
		delete ret;
		ret=NULL;
	}
	
	XMLString::release(&orgpath);
	return ret;
}
