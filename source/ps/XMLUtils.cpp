#include "XML.h"
#include "CStr.h"

#include "res/vfs.h"

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
	if (m_hFile <= 0) return -1;
	
	if (vfs_map(m_hFile, 0, m_pBuffer, m_BufferSize) != 0)
	{
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
		return new BinMemInputStream((XMLByte *)m_pBuffer, m_BufferSize,
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
	
	CStr base=m_DocName;
	const char *end=m_DocName+base.Length();
	if (strncmp(path, "../", 3) == 0)
	{
		do
			--end;
		while (end > m_DocName && *end != '/');

		while (strncmp(path, "../", 3) == 0)
		{
			path += 3;

			do
				--end;
			while (end > m_DocName && *end != '/');
		}
		
		--path;
	}
	
	if (
		(orgpath != path &&
			ret->OpenFile((base.Left(end-m_DocName)+path).c_str())!=0)
		|| (orgpath == path && ret->OpenFile(path)!=0))
	{
		delete ret;
		ret=NULL;
	}
	
success:
	XMLString::release(&orgpath);
	return ret;
}
