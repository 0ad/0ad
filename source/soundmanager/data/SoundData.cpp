/* Copyright (C) 2015 Wildfire Games.
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

#include "SoundData.h"
#include "soundmanager/SoundManager.h"

#if CONFIG2_AUDIO

#include "OggData.h"
#include "ps/CLogger.h"

DataMap CSoundData::sSoundData;

CSoundData::CSoundData()
	: m_ALBuffer(0), m_RetentionCount(0)
{
}

CSoundData::~CSoundData()
{
	AL_CHECK;
	if (m_ALBuffer != 0)
		alDeleteBuffers(1, &m_ALBuffer);
	m_ALBuffer = 0;
	AL_CHECK;
}

void CSoundData::ReleaseSoundData(CSoundData* theData)
{
	DataMap::iterator itemFind;

	if (theData->DecrementCount())
	{
		if ((itemFind = CSoundData::sSoundData.find( theData->GetFileName().string() )) != sSoundData.end())
		{
			CSoundData::sSoundData.erase(itemFind);
		}
		delete theData;
	}
}

CSoundData* CSoundData::SoundDataFromFile(const VfsPath& itemPath)
{
	Path fExt = itemPath.Extension();
	DataMap::iterator itemFind;
	CSoundData* answer = NULL;

	if ((itemFind = CSoundData::sSoundData.find(itemPath.string())) != sSoundData.end())
	{
		answer = itemFind->second;
	}
	else
	{
	  if (fExt == ".ogg")
			answer = SoundDataFromOgg(itemPath);

		if (answer && answer->IsOneShot())
		{
			CSoundData::sSoundData[itemPath.string()] = answer;
		}

	}

	return answer;
}

bool CSoundData::IsOneShot()
{
	return true;
}


CSoundData* CSoundData::SoundDataFromOgg(const VfsPath& itemPath)
{
	COggData* oggAnswer = new COggData();

	if (!oggAnswer->InitOggFile(itemPath))
	{
		LOGERROR("could not initialize ogg data at %s", itemPath.string8());
		delete oggAnswer;
		return NULL;
	}

	return oggAnswer;
}

int CSoundData::GetBufferCount()
{
	return 1;
}

const Path& CSoundData::GetFileName()
{
	return m_FileName;
}

void CSoundData::SetFileName(const Path& aName)
{
	m_FileName = aName;
}

CSoundData* CSoundData::IncrementCount()
{
	m_RetentionCount++;
	return this;
}

bool CSoundData::DecrementCount()
{
	m_RetentionCount--;

	return (m_RetentionCount <= 0);
}

unsigned int CSoundData::GetBuffer()
{
	return m_ALBuffer;
}

unsigned int* CSoundData::GetBufferPtr()
{
	return &m_ALBuffer;
}

bool CSoundData::IsStereo()
{
	return false;
}

#endif // CONFIG2_AUDIO

