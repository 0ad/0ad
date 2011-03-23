#ifndef INCLUDED_OGG
#define INCLUDED_OGG

#include "lib/external_libraries/openal.h"
#include "lib/file/vfs/vfs.h"

class OggStream
{
public:
	virtual ~OggStream() { }
	virtual ALenum Format() = 0;
	virtual ALsizei SamplingRate() = 0;

	/**
	 * @return bytes read (<= size) or a (negative) LibError
	 **/
	virtual LibError GetNextChunk(u8* buffer, size_t size) = 0;
};

typedef shared_ptr<OggStream> OggStreamPtr;

extern LibError OpenOggStream(const OsPath& pathname, OggStreamPtr& stream);

/**
 * A non-streaming OggStream (reading the whole file in advance)
 * that can cope with archived/compressed files.
 */
extern LibError OpenOggNonstream(const PIVFS& vfs, const VfsPath& pathname, OggStreamPtr& stream);

#endif // INCLUDED_OGG
