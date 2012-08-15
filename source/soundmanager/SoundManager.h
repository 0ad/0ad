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

#ifndef INCLUDED_SOUNDMANAGER_H
#define INCLUDED_SOUNDMANAGER_H

#include "soundmanager/items/ISoundItem.h"
#include "lib/file/vfs/vfs_path.h"

#include <vector>
#include <map>

typedef std::vector<ISoundItem*> ItemsList;


class CSoundManager
{
protected:

	ALuint m_ALEnvironment;
	ALCcontext* m_Context;
	ALCdevice* m_Device;
	ISoundItem* m_CurrentTune;
	ISoundItem* m_CurrentEnvirons;
	ItemsList* m_Items;
	float m_Gain;
	float m_MusicGain;
	float m_AmbientGain;
	float m_ActionGain;
	bool m_Enabled;
	long m_BufferSize;
	int m_BufferCount;
	bool m_MusicEnabled;

public:
	CSoundManager();
	virtual ~CSoundManager();

	ISoundItem* LoadItem(const VfsPath* itemPath);

	static void ScriptingInit();

	void SetMusicEnabled (bool isEnabled);

	ISoundItem* ItemFromWAV(VfsPath& fname);
	ISoundItem* ItemFromOgg(VfsPath& fname);

	ISoundItem* GetSoundItem(unsigned long itemRow);
	unsigned long Count();
	void IdleTask();
	void DeleteItem(long itemNum);
	
	void SetMemoryUsage(long bufferSize, int bufferCount);
	long GetBufferCount();
	long GetBufferSize();

	void SetMusicItem(ISoundItem* anItem);
	void SetAmbientItem(ISoundItem* anItem);
	void PlayActionItem(ISoundItem* anItem);
	void PlayGroupItem(ISoundItem* anItem, ALfloat groupGain);

	void SetMasterGain(float gain);
	void SetMusicGain(float gain);
	void SetAmbientGain(float gain);
	void SetActionGain(float gain);
	
	void SetEnabled(bool doEnable);
protected:
	void InitListener();
	virtual Status AlcInit();

};

extern CSoundManager*  g_SoundManager;


#endif // INCLUDED_SOUNDMANAGER_H
