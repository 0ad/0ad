#include "precompiled.h"
#include "ogg.h"

#include "lib/external_libraries/openal.h"
#include "lib/external_libraries/vorbis.h"

#include "lib/byte_order.h"
#include "lib/file/io/io.h"
#include "lib/file/file_system_util.h"


static LibError LibErrorFromVorbis(int err)
{
	switch(err)
	{
	case 0:
		return INFO::OK;
	case OV_HOLE:
		return ERR::AGAIN;
	case OV_EREAD:
		return ERR::IO;
	case OV_EFAULT:
		return ERR::LOGIC;
	case OV_EIMPL:
		return ERR::NOT_IMPLEMENTED;
	case OV_EINVAL:
		return ERR::INVALID_PARAM;
	case OV_ENOTVORBIS:
		return ERR::NOT_SUPPORTED;
	case OV_EBADHEADER:
		return ERR::CORRUPTED;
	case OV_EVERSION:
		return ERR::VERSION;
	case OV_ENOTAUDIO:
		return ERR::_1;
	case OV_EBADPACKET:
		return ERR::_2;
	case OV_EBADLINK:
		return ERR::_3;
	case OV_ENOSEEK:
		return ERR::_4;
	default:
		return ERR::FAIL;
	}
}


//-----------------------------------------------------------------------------

class VorbisFileAdapter
{
public:
	VorbisFileAdapter(const PFile& openedFile)
		: file(openedFile)
		, size(fs_util::FileSize(openedFile->Pathname()))
		, offset(0)
	{
	}

	static size_t Read(void* bufferToFill, size_t itemSize, size_t numItems, void* context)
	{
		VorbisFileAdapter* adapter = (VorbisFileAdapter*)context;
		const off_t sizeRequested = numItems*itemSize;
		const off_t sizeRemaining = adapter->size - adapter->offset;
		const size_t sizeToRead = (size_t)std::min(sizeRequested, sizeRemaining);

		io::Operation op(*adapter->file.get(), bufferToFill, sizeToRead, adapter->offset);
		if(io::Run(op) == INFO::OK)
		{
			adapter->offset += sizeToRead;
			return sizeToRead;
		}

		errno = EIO;
		return 0;
	}

	static int Seek(void* context, ogg_int64_t offset, int whence)
	{
		VorbisFileAdapter* adapter = (VorbisFileAdapter*)context;

		off_t origin = 0;
		switch(whence)
		{
		case SEEK_SET:
			origin = 0;
			break;
		case SEEK_CUR:
			origin = adapter->offset;
			break;
		case SEEK_END:
			origin = adapter->size+1;
			break;
			NODEFAULT;
		}

		adapter->offset = Clamp(off_t(origin+offset), off_t(0), adapter->size);
		return 0;
	}

	static int Close(void* context)
	{
		VorbisFileAdapter* adapter = (VorbisFileAdapter*)context;
		adapter->file.reset();
		return 0;	// return value is ignored
	}

	static long Tell(void* context)
	{
		VorbisFileAdapter* adapter = (VorbisFileAdapter*)context;
		return adapter->offset;
	}

private:
	PFile file;
	off_t size;
	off_t offset;
};

//-----------------------------------------------------------------------------

class VorbisBufferAdapter
{
public:
	VorbisBufferAdapter(const shared_ptr<u8>& buffer, size_t size)
		: buffer(buffer)
		, size(size)
		, offset(0)
	{
	}

	static size_t Read(void* bufferToFill, size_t itemSize, size_t numItems, void* context)
	{
		VorbisBufferAdapter* adapter = (VorbisBufferAdapter*)context;
		const off_t sizeRequested = numItems*itemSize;
		const off_t sizeRemaining = adapter->size - adapter->offset;
		const size_t sizeToRead = (size_t)std::min(sizeRequested, sizeRemaining);

		memcpy(bufferToFill, adapter->buffer.get() + adapter->offset, sizeToRead);

		adapter->offset += sizeToRead;
		return sizeToRead;
	}

	static int Seek(void* context, ogg_int64_t offset, int whence)
	{
		VorbisBufferAdapter* adapter = (VorbisBufferAdapter*)context;

		off_t origin = 0;
		switch(whence)
		{
		case SEEK_SET:
			origin = 0;
			break;
		case SEEK_CUR:
			origin = adapter->offset;
			break;
		case SEEK_END:
			origin = adapter->size+1;
			break;
			NODEFAULT;
		}

		adapter->offset = Clamp(off_t(origin+offset), off_t(0), adapter->size);
		return 0;
	}

	static int Close(void* context)
	{
		VorbisBufferAdapter* adapter = (VorbisBufferAdapter*)context;
		adapter->buffer.reset();
		return 0;	// return value is ignored
	}

	static long Tell(void* context)
	{
		VorbisBufferAdapter* adapter = (VorbisBufferAdapter*)context;
		return adapter->offset;
	}

private:
	shared_ptr<u8> buffer;
	off_t size;
	off_t offset;
};


//-----------------------------------------------------------------------------

template <typename Adapter>
class OggStreamImpl : public OggStream
{
public:
	OggStreamImpl(const Adapter& adapter)
		: adapter(adapter)
	{
	}

	LibError Open()
	{
		ov_callbacks callbacks;
		callbacks.read_func = Adapter::Read;
		callbacks.close_func = Adapter::Close;
		callbacks.seek_func = Adapter::Seek;
		callbacks.tell_func = Adapter::Tell;
		const int ret = ov_open_callbacks(&adapter, &vf, 0, 0, callbacks);
		if(ret != 0)
			WARN_RETURN(LibErrorFromVorbis(ret));

		const int link = -1;	// retrieve info for current bitstream
		info = ov_info(&vf, link);
		if(!info)
			WARN_RETURN(ERR::INVALID_HANDLE);

		return INFO::OK;
	}

	virtual ALenum Format()
	{
		return (info->channels == 1)? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	}

	virtual ALsizei SamplingRate()
	{
		return info->rate;
	}

	virtual LibError GetNextChunk(u8* buffer, size_t size)
	{
		// we may have to call ov_read multiple times because it
		// treats the buffer size "as a limit and not a request"
		size_t bytesRead = 0;
		for(;;)
		{
			const int isBigEndian = (BYTE_ORDER == BIG_ENDIAN);
			const int wordSize = sizeof(i16);
			const int isSigned = 1;
			int bitstream;	// unused
			const int ret = ov_read(&vf, (char*)buffer+bytesRead, int(size-bytesRead), isBigEndian, wordSize, isSigned, &bitstream);
			if(ret == 0)	// EOF
				return (LibError)bytesRead;
			else if(ret < 0)
				WARN_RETURN(LibErrorFromVorbis(ret));
			else	// success
			{
				bytesRead += ret;
				if(bytesRead == size)
					return (LibError)bytesRead;
			}
		}
	}

private:
	Adapter adapter;
	OggVorbis_File vf;
	vorbis_info* info;
};


//-----------------------------------------------------------------------------

LibError OpenOggStream(const OsPath& pathname, OggStreamPtr& stream)
{
	PFile file(new File);
    RETURN_ERR(file->Open(pathname, L'r'));

	shared_ptr<OggStreamImpl<VorbisFileAdapter> > tmp(new OggStreamImpl<VorbisFileAdapter>(VorbisFileAdapter(file)));
	RETURN_ERR(tmp->Open());
	stream = tmp;
	return INFO::OK;
}

LibError OpenOggNonstream(const PIVFS& vfs, const VfsPath& pathname, OggStreamPtr& stream)
{
	shared_ptr<u8> contents;
	size_t size;
	RETURN_ERR(vfs->LoadFile(pathname, contents, size));

	shared_ptr<OggStreamImpl<VorbisBufferAdapter> > tmp(new OggStreamImpl<VorbisBufferAdapter>(VorbisBufferAdapter(contents, size)));
	RETURN_ERR(tmp->Open());
	stream = tmp;
	return INFO::OK;
}
