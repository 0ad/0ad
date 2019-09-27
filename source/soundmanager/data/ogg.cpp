/* Copyright (C) 2019 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "precompiled.h"

#include "ogg.h"

#if CONFIG2_AUDIO

#include "lib/byte_order.h"
#include "lib/external_libraries/openal.h"
#include "lib/external_libraries/vorbis.h"
#include "lib/file/file_system.h"
#include "lib/file/io/io.h"
#include "lib/file/vfs/vfs_util.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"


static Status LibErrorFromVorbis(int err)
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
		return ERR::NOT_SUPPORTED;
	case OV_EINVAL:
		return ERR::INVALID_PARAM;
	case OV_ENOTVORBIS:
		return ERR::NOT_SUPPORTED;
	case OV_EBADHEADER:
		return ERR::CORRUPTED;
	case OV_EVERSION:
		return ERR::INVALID_VERSION;
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
		, size(FileSize(openedFile->Pathname()))
		, offset(0)
	{
	}

	static size_t Read(void* bufferToFill, size_t itemSize, size_t numItems, void* context)
	{
		VorbisFileAdapter* adapter = static_cast<VorbisFileAdapter*>(context);
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
		VorbisFileAdapter* adapter = static_cast<VorbisFileAdapter*>(context);

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
		VorbisFileAdapter* adapter = static_cast<VorbisFileAdapter*>(context);
		adapter->file.reset();
		return 0;	// return value is ignored
	}

	static long Tell(void* context)
	{
		VorbisFileAdapter* adapter = static_cast<VorbisFileAdapter*>(context);
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
		VorbisBufferAdapter* adapter = static_cast<VorbisBufferAdapter*>(context);

		const off_t sizeRequested = numItems*itemSize;
		const off_t sizeRemaining = adapter->size - adapter->offset;
		const size_t sizeToRead = (size_t)std::min(sizeRequested, sizeRemaining);

		memcpy(bufferToFill, adapter->buffer.get() + adapter->offset, sizeToRead);

		adapter->offset += sizeToRead;
		return sizeToRead;
	}

	static int Seek(void* context, ogg_int64_t offset, int whence)
	{
		VorbisBufferAdapter* adapter = static_cast<VorbisBufferAdapter*>(context);

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
		VorbisBufferAdapter* adapter = static_cast<VorbisBufferAdapter*>(context);
		adapter->buffer.reset();
		return 0;	// return value is ignored
	}

	static long Tell(void* context)
	{
		VorbisBufferAdapter* adapter = static_cast<VorbisBufferAdapter*>(context);
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
		m_fileEOF = false;
		info = NULL;
	}

	Status Close()
	{
		ov_clear( &vf );

		return 0;
	}

	Status Open()
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
	virtual bool atFileEOF()
	{
		return m_fileEOF;
	}

	virtual Status ResetFile()
	{
	    ov_time_seek( &vf, 0 );
	    m_fileEOF = false;
		return INFO::OK;
	}

	virtual Status GetNextChunk(u8* buffer, size_t size)
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
			if(ret == 0) {	// EOF
				m_fileEOF = true;
				return (Status)bytesRead;
			}
			else if(ret < 0)
				WARN_RETURN(LibErrorFromVorbis(ret));
			else	// success
			{
				bytesRead += ret;
				if(bytesRead == size)
					return (Status)bytesRead;
			}
		}
	}

private:
	Adapter adapter;
	OggVorbis_File vf;
	vorbis_info* info;
	bool m_fileEOF;
};


//-----------------------------------------------------------------------------

Status OpenOggStream(const OsPath& pathname, OggStreamPtr& stream)
{
	PFile file(new File);
    RETURN_STATUS_IF_ERR(file->Open(pathname, L'r'));

	shared_ptr<OggStreamImpl<VorbisFileAdapter> > tmp(new OggStreamImpl<VorbisFileAdapter>(VorbisFileAdapter(file)));
	RETURN_STATUS_IF_ERR(tmp->Open());
	stream = tmp;
	return INFO::OK;
}

Status OpenOggNonstream(const PIVFS& vfs, const VfsPath& pathname, OggStreamPtr& stream)
{
	shared_ptr<u8> contents;
	size_t size;
	RETURN_STATUS_IF_ERR(vfs->LoadFile(pathname, contents, size));

	shared_ptr<OggStreamImpl<VorbisBufferAdapter> > tmp(new OggStreamImpl<VorbisBufferAdapter>(VorbisBufferAdapter(contents, size)));
	RETURN_STATUS_IF_ERR(tmp->Open());
	stream = tmp;
	return INFO::OK;
}

#endif	// CONFIG2_AUDIO

