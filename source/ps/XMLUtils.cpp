#include "precompiled.h"

#include "XML.h"
#include "CStr.h"
#include "CLogger.h"
#include "posix.h"		// ptrdiff_t

#include "res/vfs.h"


#ifdef _MSC_VER
#pragma comment(lib, "xerces-c_2.lib")
#endif

XERCES_CPP_NAMESPACE_USE

CStr XMLTranscode(const XMLCh* xmltext)
{
	char* str=XMLString::transcode((const XMLCh *)xmltext);
	CStr result(str);
	XMLString::release(&str);
	return result;
}

XMLCh *XMLTranscode(const CStr &str)
{
	const char *cstr=(const char *)str;
	return XMLString::transcode(cstr);
}

int CVFSInputSource::OpenFile(const char *path)
{
	debug_out("CVFSInputSource::OpenFile(): opening file %s.\n", path);

	m_hFile=vfs_open(path);
	if (m_hFile <= 0)
	{
		LOG(ERROR, "CVFSInputSource: file %s couldn't be opened (vfs_open)\n", path);
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

InputSource *CVFSEntityResolver::resolveEntity(const XMLCh *const publicId,
	const XMLCh *const systemId)
{
	CVFSInputSource *ret=new CVFSInputSource();
	char *path=XMLString::transcode(systemId);
	char *orgpath=path;
	
	CStr abspath=m_DocName;
	const char *end=m_DocName+abspath.Length();
	if (strncmp(path, "..", 2) == 0 && (path[2] == '/' || path[2] == '\\'))
	{
		do
			--end;
		while (end > m_DocName && (*end != '/' && *end != '\\'));

		while (strncmp(path, "..", 2) == 0 && (path[2] == '/' || path[2] == '\\'))
		{
			path += 3;

			do
				--end;
			while (end > m_DocName && (*end != '/' && *end != '\\'));
		}
		
		--path;
		
		const ptrdiff_t d = end-m_DocName;
		abspath=abspath.Left((long)d)+path;
		
		int pos=0;
		if (abspath.Find('\\') != -1)
		{
			LOG(WARNING, "While resolving XML entities for %s: path %s [%s] contains non-portable path separator \\", m_DocName, orgpath, abspath.c_str());
			abspath.Replace("\\", "/");
		}
	}
	
	if ((orgpath != path &&	ret->OpenFile(abspath)!=0)
		|| (orgpath == path && ret->OpenFile(path)!=0))
	{
		delete ret;
		ret=NULL;
	}
	
	XMLString::release(&orgpath);
	return ret;
}
