#include "precompiled.h"
#include "real_directory.h"

#include "lib/path_util.h"
#include "lib/file/file.h"
#include "lib/file/io/io.h"


RealDirectory::RealDirectory(const Path& path, size_t priority, size_t flags)
	: m_path(path), m_priority(priority), m_flags(flags)
{
}


/*virtual*/ size_t RealDirectory::Precedence() const
{
	return 1u;
}


/*virtual*/ char RealDirectory::LocationCode() const
{
	return 'F';
}


/*virtual*/ LibError RealDirectory::Load(const std::string& name, const shared_ptr<u8>& buf, size_t size) const
{
	PIFile file = CreateFile_Posix();
	RETURN_ERR(file->Open(m_path/name, 'r'));

	RETURN_ERR(io_ReadAligned(file, 0, buf.get(), size));
	return INFO::OK;
}


LibError RealDirectory::Store(const std::string& name, const shared_ptr<u8>& fileContents, size_t size)
{
	const Path pathname(m_path/name);

	{
		PIFile file = CreateFile_Posix();
		RETURN_ERR(file->Open(pathname, 'w'));
		RETURN_ERR(io_WriteAligned(file, 0, fileContents.get(), size));
	}

	// io_WriteAligned pads the file; we need to truncate it to the actual
	// length. ftruncate can't be used because Windows' FILE_FLAG_NO_BUFFERING
	// only allows resizing to sector boundaries, so the file must first
	// be closed.
	truncate(pathname.external_file_string().c_str(), (off_t)size);

	return INFO::OK;
}


void RealDirectory::Watch()
{
	//m_watch = CreateWatch(Path().external_file_string().c_str());
}


PRealDirectory CreateRealSubdirectory(const PRealDirectory& realDirectory, const std::string& subdirectoryName)
{
	const Path path(realDirectory->GetPath()/subdirectoryName);
	return PRealDirectory(new RealDirectory(path, realDirectory->Priority(), realDirectory->Flags()));
}
