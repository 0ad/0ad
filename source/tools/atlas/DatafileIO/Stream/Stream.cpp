#include "stdafx.h"

#include "Stream.h"

#include <cassert>
#include <string>

using namespace DatafileIO;

bool InputStream::AcquireBuffer(void*& buffer, size_t& size, size_t max_size)
{
	std::string data;
	const size_t tempBufSize = 65536;
	static char tempBuffer[tempBufSize];
	size = 0;
	size_t bytesLeft = max_size;
	size_t bytesRead;
	while (bytesLeft > 0 && (bytesRead = Read(tempBuffer, std::min(bytesLeft, tempBufSize))) != 0)
	{
		size += bytesRead;
		bytesLeft -= bytesRead;
		data += std::string(tempBuffer, bytesRead);
	}

	buffer = new char[size];
	memcpy(buffer, data.data(), size);

	return true;
}

void InputStream::ReleaseBuffer(void* buffer)
{
	delete[] (char*)buffer;
}
