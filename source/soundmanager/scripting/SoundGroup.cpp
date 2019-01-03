/* Copyright (C) 2019 Wildfire Games.
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

#include "SoundGroup.h"
#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "lib/rand.h"
#include "ps/Game.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "ps/Util.h"
#include "ps/XML/Xeromyces.h"
#include "soundmanager/items/ISoundItem.h"
#include "soundmanager/SoundManager.h"

#include <algorithm>

extern CGame *g_Game;

#if CONFIG2_AUDIO
static float RandFloat(float min, float max)
{
	return rand(min * 100.f, max * 100.f) / 100.f;
}
#endif

void CSoundGroup::SetGain(float gain)
{
	m_Gain = std::min(gain, 1.0f);
}

void CSoundGroup::SetDefaultValues()
{
	m_CurrentSoundIndex = 0;
	m_Flags = 0;
	m_CurTime = 0.f;
	m_Gain = 0.7f;
	m_Pitch = 1.f;
	m_Priority = 60.f;
	m_PitchUpper = 1.1f;
	m_PitchLower = 0.9f;
	m_GainUpper = 1.f;
	m_GainLower = 0.8f;
	m_ConeOuterGain = 0.f;
	m_ConeInnerAngle = 360.f;
	m_ConeOuterAngle = 360.f;
	m_Decay = 3.f;
	m_IntensityThreshold = 3.f;
}

CSoundGroup::CSoundGroup()
{
	SetDefaultValues();
}

CSoundGroup::CSoundGroup(const VfsPath& pathnameXML)
{
	SetDefaultValues();
	LoadSoundGroup(pathnameXML);
}

CSoundGroup::~CSoundGroup()
{
	// clean up all the handles from this group.
	ReleaseGroup();
}

float CSoundGroup::RadiansOffCenter(const CVector3D& position, bool& onScreen, float& itemRollOff)
{
	const int screenWidth = g_Game->GetView()->GetCamera()->GetViewPort().m_Width;
	const int screenHeight = g_Game->GetView()->GetCamera()->GetViewPort().m_Height;
	const float xBufferSize = screenWidth * 0.1f;
	const float yBufferSize = 15.f;
	const float radianCap = M_PI / 3.f;

	float x, y;
	g_Game->GetView()->GetCamera()->GetScreenCoordinates(position, x, y);

	onScreen = true;
	float answer = 0.f;
	if (x < -xBufferSize)
	{
		onScreen = false;
		answer = -radianCap;
	}
	else if (x > screenWidth + xBufferSize)
	{
		onScreen = false;
		answer = radianCap;
	}
	else
	{
		if (x < 0 || x > screenWidth)
			itemRollOff = 0.5;

		answer = radianCap * (x * 2 / screenWidth - 1);
	}

	if (y < -yBufferSize)
	{
		onScreen = false;
	}
	else if (y > screenHeight + yBufferSize)
	{
		onScreen = false;
	}
	else if (y < 0 || y > screenHeight)
	{
		itemRollOff = 0.5;
	}

	return answer;
}

void CSoundGroup::UploadPropertiesAndPlay(size_t index, const CVector3D& position, entity_id_t source)
{
#if !CONFIG2_AUDIO
	UNUSED2(index);
	UNUSED2(position);
	UNUSED2(source);
#else
	if (!g_SoundManager)
		return;

	bool isOnscreen = false;
	ALfloat itemRollOff = 0.1f;
	float offset = RadiansOffCenter(position, isOnscreen, itemRollOff);

	if (!isOnscreen && !TestFlag(eDistanceless) && !TestFlag(eOmnipresent))
		return;

	if (m_SoundGroups.empty())
		Reload();

	if (m_SoundGroups.size() <= index)
		return;

	CSoundData* sndData = m_SoundGroups[index];
	if (!sndData)
		return;

	ISoundItem* hSound = static_cast<CSoundManager*>(g_SoundManager)->ItemForEntity(source, sndData);
	if (!hSound)
		return;

	if (!TestFlag(eOmnipresent))
	{
		CVector3D origin = g_Game->GetView()->GetCamera()->GetOrientation().GetTranslation();
		float sndDist = origin.Y;
		float itemDist = (position - origin).Length();

		if (sndDist * 2 < itemDist)
			sndDist = itemDist;

		if (TestFlag(eDistanceless))
			itemRollOff = 0;

		if (sndData->IsStereo())
			LOGWARNING("OpenAL: stereo sounds can't be positioned: %s", sndData->GetFileName().string8());

		hSound->SetLocation(CVector3D(sndDist * sin(offset), 0, -sndDist * cos(offset)));
		hSound->SetRollOff(itemRollOff);
	}

	if (TestFlag(eRandPitch))
		hSound->SetPitch(RandFloat(m_PitchLower, m_PitchUpper));
	else
		hSound->SetPitch(m_Pitch);

	if (TestFlag(eRandGain))
		m_Gain = RandFloat(m_GainLower, m_GainUpper);

	hSound->SetCone(m_ConeInnerAngle, m_ConeOuterAngle, m_ConeOuterGain);
	static_cast<CSoundManager*>(g_SoundManager)->PlayGroupItem(hSound, m_Gain);
#endif // !CONFIG2_AUDIO
}

static void HandleError(const std::wstring& message, const VfsPath& pathname, Status err)
{
	// Open failed because sound is disabled (don't log this)
	if (err == ERR::AGAIN)
		return;

	LOGERROR("%s: pathname=%s, error=%s", utf8_from_wstring(message), pathname.string8(), utf8_from_wstring(ErrorString(err)));
}

void CSoundGroup::PlayNext(const CVector3D& position, entity_id_t source)
{
	if (m_Filenames.empty())
		return;

	m_CurrentSoundIndex = rand(0, m_Filenames.size());
	UploadPropertiesAndPlay(m_CurrentSoundIndex, position, source);
}

void CSoundGroup::Reload()
{
	m_CurrentSoundIndex = 0;
#if CONFIG2_AUDIO
	ReleaseGroup();

	if (!g_SoundManager)
		return;

	for (const std::wstring& filename : m_Filenames)
	{
		VfsPath absolutePath = m_Filepath / filename;
		CSoundData* itemData = CSoundData::SoundDataFromFile(absolutePath);
		if (!itemData)
			HandleError(L"error loading sound", absolutePath, ERR::FAIL);
		else
			m_SoundGroups.push_back(itemData->IncrementCount());
	}

	if (TestFlag(eRandOrder))
		random_shuffle(m_SoundGroups.begin(), m_SoundGroups.end());
#endif
}

void CSoundGroup::ReleaseGroup()
{
#if CONFIG2_AUDIO
	for (CSoundData* soundGroup : m_SoundGroups)
		CSoundData::ReleaseSoundData(soundGroup);

	m_SoundGroups.clear();
#endif
}

void CSoundGroup::Update(float UNUSED(TimeSinceLastFrame))
{
}

bool CSoundGroup::LoadSoundGroup(const VfsPath& pathnameXML)
{
	CXeromyces XeroFile;
	if (XeroFile.Load(g_VFS, pathnameXML, "sound_group") != PSRETURN_OK)
	{
		HandleError(L"error loading file", pathnameXML, ERR::FAIL);
		return false;
	}

#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	EL(soundgroup);
	EL(gain);
	EL(looping);
	EL(omnipresent);
	EL(heardby);
	EL(distanceless);
	EL(pitch);
	EL(priority);
	EL(randorder);
	EL(randgain);
	EL(randpitch);
	EL(conegain);
	EL(coneinner);
	EL(coneouter);
	EL(sound);
	EL(gainupper);
	EL(gainlower);
	EL(pitchupper);
	EL(pitchlower);
	EL(path);
	EL(threshold);
	EL(decay);
#undef EL

	XMBElement root = XeroFile.GetRoot();

	if (root.GetNodeName() != el_soundgroup)
	{
		LOGERROR("Invalid SoundGroup format (unrecognised root element '%s')", XeroFile.GetElementString(root.GetNodeName()).c_str());
		return false;
	}

	XERO_ITER_EL(root, child)
	{
		int child_name = child.GetNodeName();

		if (child_name == el_gain)
		{
			SetGain(child.GetText().ToFloat());
		}
		else if (child_name == el_looping)
		{
			if (child.GetText().ToInt() == 1)
				SetFlag(eLoop);
		}
		else if (child_name == el_omnipresent)
		{
			if (child.GetText().ToInt() == 1)
				SetFlag(eOmnipresent);
		}
		else if (child_name == el_heardby)
		{
			if (child.GetText().FindInsensitive("owner") == 0)
				SetFlag(eOwnerOnly);
		}
		else if (child_name == el_distanceless)
		{
			if (child.GetText().ToInt() == 1)
				SetFlag(eDistanceless);
		}
		else if (child_name == el_pitch)
		{
			this->m_Pitch = child.GetText().ToFloat();
		}
		else if (child_name == el_priority)
		{
			this->m_Priority = child.GetText().ToFloat();
		}
		else if (child_name == el_randorder)
		{
			if (child.GetText().ToInt() == 1)
				SetFlag(eRandOrder);
		}
		else if (child_name == el_randgain)
		{
			if (child.GetText().ToInt() == 1)
				SetFlag(eRandGain);
		}
		else if (child_name == el_gainupper)
		{
			this->m_GainUpper = child.GetText().ToFloat();
		}
		else if (child_name == el_gainlower)
		{
			this->m_GainLower = child.GetText().ToFloat();
		}
		else if (child_name == el_randpitch)
		{
			if (child.GetText().ToInt() == 1)
				SetFlag(eRandPitch);
		}
		else if (child_name == el_pitchupper)
		{
			this->m_PitchUpper = child.GetText().ToFloat();
		}
		else if (child_name == el_pitchlower)
		{
			this->m_PitchLower = child.GetText().ToFloat();
		}
		else if (child_name == el_conegain)
		{
			this->m_ConeOuterGain = child.GetText().ToFloat();
		}
		else if (child_name == el_coneinner)
		{
			this->m_ConeInnerAngle = child.GetText().ToFloat();
		}
		else if (child_name == el_coneouter)
		{
			this->m_ConeOuterAngle = child.GetText().ToFloat();
		}
		else if (child_name == el_sound)
		{
			this->m_Filenames.push_back(child.GetText().FromUTF8());
		}
		else if (child_name == el_path)
		{
			m_Filepath = child.GetText().FromUTF8();
		}
		else if (child_name == el_threshold)
		{
			m_IntensityThreshold = child.GetText().ToFloat();
		}
		else if (child_name == el_decay)
		{
			m_Decay = child.GetText().ToFloat();
		}
	}
	return true;
}
