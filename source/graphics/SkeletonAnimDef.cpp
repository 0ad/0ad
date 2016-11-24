/* Copyright (C) 2009 Wildfire Games.
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

/*
 * Raw description of a skeleton animation
 */

#include "precompiled.h"

#include "SkeletonAnimDef.h"
#include "maths/MathUtil.h"
#include "ps/FileIo.h"


///////////////////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimDef constructor
CSkeletonAnimDef::CSkeletonAnimDef() : m_FrameTime(0), m_NumKeys(0), m_NumFrames(0), m_Keys(0)
{
}

///////////////////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimDef destructor
CSkeletonAnimDef::~CSkeletonAnimDef()
{
	delete[] m_Keys;
}

///////////////////////////////////////////////////////////////////////////////////////////
// BuildBoneMatrices: build matrices for all bones at the given time (in MS) in this
// animation
void CSkeletonAnimDef::BuildBoneMatrices(float time, CMatrix3D* matrices, bool loop) const
{
	float fstartframe = time/m_FrameTime;
	size_t startframe = (size_t)(int)(time/m_FrameTime);
	float deltatime = fstartframe-startframe;

	startframe %= m_NumFrames;

	size_t endframe = startframe + 1;
	endframe %= m_NumFrames;

	if (!loop && endframe == 0)
	{
		// This might be something like a death animation, and interpolating
		// between the final frame and the initial frame is wrong, because they're
		// totally different. So if we've looped around to endframe==0, just display
		// the animation's final frame with no interpolation.
		for (size_t i = 0; i < m_NumKeys; i++)
		{
			const Key& key = GetKey(startframe, i);
			matrices[i].SetIdentity();
			matrices[i].Rotate(key.m_Rotation);
			matrices[i].Translate(key.m_Translation);
		}
	}
	else
	{
		for (size_t i = 0; i < m_NumKeys; i++)
		{
			const Key& startkey = GetKey(startframe, i);
			const Key& endkey = GetKey(endframe, i);

			CVector3D trans = Interpolate(startkey.m_Translation, endkey.m_Translation, deltatime);
			// TODO: is slerp the best thing to use here?
			CQuaternion rot;
			rot.Slerp(startkey.m_Rotation, endkey.m_Rotation, deltatime);

			rot.ToMatrix(matrices[i]);
			matrices[i].Translate(trans);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// Load: try to load the anim from given file; return a new anim if successful
CSkeletonAnimDef* CSkeletonAnimDef::Load(const VfsPath& filename)
{
	CFileUnpacker unpacker;
	unpacker.Read(filename,"PSSA");

	// check version
	if (unpacker.GetVersion()<FILE_READ_VERSION) {
		throw PSERROR_File_InvalidVersion();
	}

	// unpack the data
	CSkeletonAnimDef* anim=new CSkeletonAnimDef;
	try {
		CStr name; // unused - just here to maintain compatibility with the animation files
		unpacker.UnpackString(name);
		unpacker.UnpackRaw(&anim->m_FrameTime,sizeof(anim->m_FrameTime));
		anim->m_NumKeys = unpacker.UnpackSize();
		anim->m_NumFrames = unpacker.UnpackSize();
		anim->m_Keys=new Key[anim->m_NumKeys*anim->m_NumFrames];
		unpacker.UnpackRaw(anim->m_Keys,anim->m_NumKeys*anim->m_NumFrames*sizeof(Key));
	} catch (PSERROR_File&) {
		delete anim;
		throw;
	}

	return anim;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Save: try to save anim to file
void CSkeletonAnimDef::Save(const VfsPath& pathname,const CSkeletonAnimDef* anim)
{
	CFilePacker packer(FILE_VERSION, "PSSA");

	// pack up all the data
	packer.PackString("");
	packer.PackRaw(&anim->m_FrameTime,sizeof(anim->m_FrameTime));
	const size_t numKeys = anim->m_NumKeys;
	packer.PackSize(numKeys);
	const size_t numFrames = anim->m_NumFrames;
	packer.PackSize(numFrames);
	packer.PackRaw(anim->m_Keys,numKeys*numFrames*sizeof(Key));

	// now write it
	packer.Write(pathname);
}
