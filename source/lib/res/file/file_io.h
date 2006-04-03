#ifndef FILE_IO_H__
#define FILE_IO_H__

extern void file_io_init();
extern void file_io_shutdown();

extern LibError file_io_get_buf(FileIOBuf* pbuf, size_t size,
	const char* atom_fn, uint file_flags, FileIOCB cb);

#endif	// #ifndef FILE_IO_H__
