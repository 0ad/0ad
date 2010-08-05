#ifndef INCLUDED_OGG
#define INCLUDED_OGG

#include "lib/external_libraries/openal.h"

class OggStream
{
public:
	virtual ~OggStream() { }
	virtual ALenum Format() = 0;
	virtual ALsizei SamplingRate() = 0;
	virtual LibError GetNextChunk(u8* buffer, size_t size) = 0;
};

typedef shared_ptr<OggStream> OggStreamPtr;

extern LibError OpenOggStream(const fs::wpath& pathname, OggStreamPtr& stream);

#endif // INCLUDED_OGG
