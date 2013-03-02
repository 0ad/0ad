/* Copyright (C) 2012 Wildfire Games.
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
#ifndef INCLUDED_OGG
#define INCLUDED_OGG

#include "lib/config2.h"

#if CONFIG2_AUDIO

#include "lib/external_libraries/openal.h"
#include "lib/file/vfs/vfs.h"

class OggStream
{
public:
	virtual ~OggStream() { }
	virtual ALenum Format() = 0;
	virtual ALsizei SamplingRate() = 0;
	virtual bool atFileEOF() = 0;
	virtual Status ResetFile() = 0;
  virtual Status Close() = 0;

	/**
	 * @return bytes read (<= size) or a (negative) Status
	 **/
	virtual Status GetNextChunk(u8* buffer, size_t size) = 0;
};

typedef shared_ptr<OggStream> OggStreamPtr;

extern Status OpenOggStream(const OsPath& pathname, OggStreamPtr& stream);

/**
 * A non-streaming OggStream (reading the whole file in advance)
 * that can cope with archived/compressed files.
 */
extern Status OpenOggNonstream(const PIVFS& vfs, const VfsPath& pathname, OggStreamPtr& stream);

#endif	// CONFIG2_AUDIO

#endif // INCLUDED_OGG
