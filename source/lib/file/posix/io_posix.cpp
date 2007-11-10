/**
 * =========================================================================
 * File        : io_posix.cpp
 * Project     : 0 A.D.
 * Description : lightweight POSIX aio wrapper
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "io_posix.h"

#include "lib/file/filesystem.h"
#include "lib/file/path.h"
#include "lib/file/file_stats.h"
#include "lib/posix/posix_aio.h"


//-----------------------------------------------------------------------------

File_Posix::File_Posix()
{
}


File_Posix::~File_Posix()
{
	Close();
}


LibError File_Posix::Open(const char* P_pathname, char mode, uint flags)
{
	debug_assert(mode == 'w' || mode == 'r');
	debug_assert(flags <= FILE_FLAG_ALL);

	m_pathname = path_UniqueCopy(P_pathname);
	m_mode = mode;
	m_flags = flags;

	char N_pathname[PATH_MAX];
	(void)file_make_full_native_path(P_pathname, N_pathname);

	int oflag = (mode == 'r')? O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;

#if OS_WIN
	if(flags & FILE_TEXT)
		oflag |= O_TEXT_NP;
	else
		oflag |= O_BINARY_NP;

	// if AIO is disabled at user's behest, inform wposix.
	if(flags & FILE_NO_AIO)
		oflag |= O_NO_AIO_NP;
#endif

	m_fd = open(N_pathname, oflag, S_IRWXO|S_IRWXU|S_IRWXG);
	if(m_fd < 0)
		RETURN_ERR(ERR::FILE_ACCESS);

	return INFO::OK;
}


void File_Posix::Close()
{
	close(m_fd);
	m_fd = 0;
}


LibError File_Posix::Validate() const
{
	if(path_UniqueCopy(m_pathname) != m_pathname)
		WARN_RETURN(ERR::_1);
	if((m_mode != 'w' && m_mode != 'r'))
		WARN_RETURN(ERR::_2);
	if(m_flags > FILE_FLAG_ALL)
		WARN_RETURN(ERR::_3);
	// >= 0x100 is not necessarily bogus, but suspicious.
	if(!(3 <= m_fd && m_fd < 0x100))
		WARN_RETURN(ERR::_4);

	return INFO::OK;
}


//-----------------------------------------------------------------------------

class Io_Posix::Impl
{
public:
	Impl()
	{
		memset(&m_aiocb, 0, sizeof(m_aiocb));
	}

	LibError Issue(File_Posix& file, off_t ofs, IoBuf buf, size_t size)
	{
		debug_printf("FILE| Issue ofs=0x%X size=0x%X\n", ofs, size);

		m_aiocb.aio_lio_opcode = (file.Mode() == 'w')? LIO_WRITE : LIO_READ;
		m_aiocb.aio_buf        = (volatile void*)buf;
		m_aiocb.aio_fildes     = file.Handle();
		m_aiocb.aio_offset     = ofs;
		m_aiocb.aio_nbytes     = size;
		struct sigevent* sig = 0;	// no notification signal
		if(lio_listio(LIO_NOWAIT, (aiocb**)&m_aiocb, 1, sig) != 0)
			return LibError_from_errno();

		return INFO::OK;
	}

	LibError Validate() const
	{
		if(debug_is_pointer_bogus((void*)m_aiocb.aio_buf))
			WARN_RETURN(ERR::_2);
		const int opcode = m_aiocb.aio_lio_opcode;
		if(opcode != LIO_WRITE && opcode != LIO_READ && opcode != LIO_NOP)
			WARN_RETURN(ERR::_3);
		// all other aiocb fields have no invariants we could check.
		return INFO::OK;
	}

	LibError Status() const
	{
		debug_assert(Validate() == INFO::OK);

		errno = 0;
		int ret = aio_error(&m_aiocb);
		if(ret == EINPROGRESS)
			return INFO::IO_PENDING;
		if(ret == 0)
			return INFO::IO_COMPLETE;

		return LibError_from_errno();
	}

	LibError WaitUntilComplete(IoBuf& buf, size_t& size)
	{
		debug_printf("FILE| Wait io=%p\n", this);
		debug_assert(Validate() == INFO::OK);

		// wait for transfer to complete.
		while(aio_error(&m_aiocb) == EINPROGRESS)
			aio_suspend((aiocb**)&m_aiocb, 1, (timespec*)0);	// wait indefinitely

		// query number of bytes transferred (-1 if the transfer failed)
		const ssize_t bytes_transferred = aio_return(&m_aiocb);
		debug_printf("FILE| bytes_transferred=%d aio_nbytes=%u\n", bytes_transferred, m_aiocb.aio_nbytes);

		buf = (IoBuf)m_aiocb.aio_buf;	// cast from volatile void*
		size = bytes_transferred;
		return INFO::OK;
	}

private:
	aiocb m_aiocb;
};


//-----------------------------------------------------------------------------

Io_Posix::Io_Posix()
: impl(new Impl)
{
}

Io_Posix::~Io_Posix()
{
}

LibError Io_Posix::Issue(File_Posix& file, off_t ofs, IoBuf buf, size_t size)
{
	return impl.get()->Issue(file, ofs, buf, size);
}

LibError Io_Posix::Status() const
{
	return impl.get()->Status();
}

LibError Io_Posix::WaitUntilComplete(IoBuf& buf, size_t& size)
{
	return impl.get()->WaitUntilComplete(buf, size);
}


//-----------------------------------------------------------------------------

LibError io_posix_Synchronous(File_Posix& file, off_t ofs, IoBuf buf, size_t size)
{
	const int fd = file.Handle();
	const bool isWrite = (file.Mode() == 'w');

	ScopedIoMonitor monitor;

	lseek(fd, ofs, SEEK_SET);

	errno = 0;
	void* dst = (void*)buf;
	const ssize_t ret = isWrite? write(fd, dst, size) : read(fd, dst, size);
	if(ret < 0)
		return LibError_from_errno();

	const size_t totalTransferred = (size_t)ret;
	if(totalTransferred != size)
		WARN_RETURN(ERR::IO);

	monitor.NotifyOfSuccess(FI_LOWIO, isWrite? FO_WRITE : FO_READ, totalTransferred);
	return INFO::OK;
}
