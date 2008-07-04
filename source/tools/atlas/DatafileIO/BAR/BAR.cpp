#include "precompiled.h"

#include "BAR.h"
#include "Util.h"
#include "Stream/Stream.h"
#include "Stream/Memory.h"

//for :25 strncmp and :81 memset
#include <string.h>

#include <cassert>

using namespace DatafileIO;

BARReader::BARReader(SeekableInputStream& stream)
: m_Stream(stream)
{
}

#define CHECK(expr) if (!(expr)) { assert(!(expr)); return false; }

bool BARReader::Initialise()
{
	assert(m_FileList.size() == 0); // Only call Init once

	char head[4];
	m_Stream.Read(head, 4);
	CHECK(strncmp(head, "ESPN", 4) == 0);

	uint32_t unknown;
	m_Stream.Read(&unknown, 4);
	CHECK(unknown == 2);

	m_Stream.Read(&unknown, 4);
	CHECK(unknown == 0x44332211);

	for (int i = 0; i < 66; ++i)
	{
		m_Stream.Read(&unknown, 4);
		CHECK(unknown == 0);
	}

	m_Stream.Read(&unknown, 4); // TODO: checksum?

	uint32_t numFiles, filetableOffset;
	m_Stream.Read(&numFiles, 4);
	m_Stream.Read(&filetableOffset, 4);

	m_Stream.Read(&unknown, 4);
	CHECK(unknown == 0);

	m_Stream.Seek(filetableOffset, Stream::FROM_START);

	utf16string rootName = ReadUString(m_Stream);
	uint32_t numRootFiles;
	m_Stream.Read(&numRootFiles, 4);
	CHECK(numRootFiles == numFiles);

	m_FileList.reserve(numFiles);
	for (uint32_t i = 0; i < numFiles; ++i)
	{
		BAREntry file;

		uint32_t offset, length0, length1;
		m_Stream.Read(&offset, 4);
		m_Stream.Read(&length0, 4);
		m_Stream.Read(&length1, 4);
		CHECK(length0 == length1); // ??

		file.offset = offset;
		file.filesize = length0;

		// Ranges: 1995-2005, 1-12, 0-5, 1-31, 0+10-23, 0-59, 0-59, 0-999
		m_Stream.Read(&file.modified.year, 2);
		m_Stream.Read(&file.modified.month, 2);
		m_Stream.Read(&file.modified.dayofweek, 2);
		m_Stream.Read(&file.modified.day, 2);
		m_Stream.Read(&file.modified.hour, 2);
		m_Stream.Read(&file.modified.minute, 2);
		m_Stream.Read(&file.modified.second, 2);
		m_Stream.Read(&file.modified.msecond, 2);

		if (file.modified.year == 0xCCCC) // no date specified
			memset(&file.modified, 0, sizeof(file.modified));

		file.filename = rootName + ReadUString(m_Stream);

		m_FileList.push_back(file);
	}

	// TODO: check this is really EOF

	return true;
}

struct buffer_releaser
{
	SeekableInputStream* stream;

};

SeekableInputStream* BARReader::GetFile(const BAREntry& file) const
{
	void* buffer;
	size_t size;
	m_Stream.Seek((off_t)file.offset, Stream::FROM_START);
	m_Stream.AcquireBuffer(buffer, size, file.filesize);
	return new SeekableInputStream_mem((char*)buffer, size, new SeekableInputStream_mem::Releaser_StreamBuffer(&m_Stream));
}

void BARReader::TransferFile(const BAREntry& file, OutputStream& stream) const
{
	m_Stream.Seek((off_t)file.offset, Stream::FROM_START);
	const size_t bufSize = 128*1024; // people 
	static char* buffer[bufSize];
	size_t bytesLeft = file.filesize;
	while (bytesLeft)
	{
		size_t bytesRead = m_Stream.Read(buffer, std::min(bufSize, bytesLeft));
		stream.Write(buffer, bytesRead);
		bytesLeft -= bytesRead;
		if (bytesRead == 0)
			break;
	}
}
