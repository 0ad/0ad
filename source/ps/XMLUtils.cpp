#include "precompiled.h"

#include "XML.h"

#undef new // if it was redefined for leak detection, since xerces doesn't like it

#include "CStr.h"
#include "CLogger.h"
#include "posix.h"		// ptrdiff_t

#include "res/vfs.h"

/*
// but static Xerces => tons of warnings due to missing debug info,
// and warnings about invalid pointers (conflicting CRT heaps?) in parser => allow for now
#ifndef XERCES_STATIC_LIB
#error "need to define XERCES_STATIC_LIB in project options (so that Xerces uses the same CRT as the other libs)"
#endif
*/

#ifdef _MSC_VER
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
#endif		// _MSC_VER

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

int CVFSInputSource::OpenFile(const char *path)
{
	debug_out("CVFSInputSource::OpenFile(): opening file %s.\n", path);

	m_hFile=vfs_open(path);
	if (m_hFile <= 0)
	{
		LOG(ERROR, "CVFSInputSource: file %s couldn't be opened (vfs_open: %lld)\n", path, m_hFile);
		return -1;
	}
	
	int err;
	if ((err=vfs_map(m_hFile, 0, m_pBuffer, m_BufferSize)) != 0)
	{
		LOG(ERROR, "CVFSInputSource: file %s couldn't be opened (vfs_map: %d)\n", path, err);
		vfs_close(m_hFile);
		m_hFile=0;
		return -1;
	}
	
	XMLCh *sysId=XMLString::transcode(path);
	setSystemId(sysId);
	XMLString::release(&sysId);
	
	return 0;
}

CVFSInputSource::~CVFSInputSource()
{
	if (m_hFile > 0)
	{
		vfs_unmap(m_hFile);
		vfs_close(m_hFile);
	}
}

BinInputStream *CVFSInputSource::makeStream() const
{
	if (m_hFile > 0)
	{
		return new BinMemInputStream((XMLByte *)m_pBuffer, (unsigned int)m_BufferSize,
			BinMemInputStream::BufOpt_Reference);
	}
	else
		return NULL;
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

InputSource *CVFSEntityResolver::resolveEntity(const XMLCh *const publicId,
	const XMLCh *const systemId)
{
	CVFSInputSource *ret=new CVFSInputSource();
	char *path=XMLString::transcode(systemId);
	char *orgpath=path;
	
	char abspath[VFS_MAX_PATH];
	const char *end=strchr(m_DocName, '\0');
	const char *orgend=end;

	if (IS_PATH_SEP(*path))
		path++;
	else
	{
		// We know that we have a relative path here:
		// - Remove the file name
		// - If we have a ../ components - remove them and remove one component
		// off the end of the document path for each ../ component
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
		
		memcpy(abspath, m_DocName, prefixlen);
		strncpy(abspath+prefixlen, path, VFS_MAX_PATH-prefixlen);
		// strncpy might not have terminated, if path was too long
		abspath[VFS_MAX_PATH-1]=0;
		
		path=abspath;
	}

	LOG(NORMAL, "EntityResolver: path \"%s\" translated to \"%s\"", orgpath, path);

	char *pos=path;		
	if ((pos=strchr(pos, '\\')) != NULL)
	{
		LOG(WARNING, "While resolving XML entities for %s: path %s [%s] contains non-portable path separator \\", m_DocName, orgpath, path);
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
