#include "stdafx.h"

#include "Util.h"
#include "Stream/Stream.h"

using namespace DatafileIO;

utf16string DatafileIO::ReadUString(InputStream& stream)
{
	uint32_t length;
	stream.Read(&length, 4);

	utf16string ret;
	ret.resize(length);
	stream.Read(&ret[0], length*2);

	return ret;
}

void DatafileIO::WriteUString(OutputStream& stream, const utf16string& string)
{
	uint32_t length = (uint32_t)string.length();
	stream.Write(&length, 4);
	stream.Write((utf16_t*)&string[0], length*2);
}
