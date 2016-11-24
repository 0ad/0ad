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

#ifndef INCLUDED_SKELETONANIMDEF
#define INCLUDED_SKELETONANIMDEF

#include "maths/Vector3D.h"
#include "maths/Quaternion.h"
#include "lib/file/vfs/vfs_path.h"

////////////////////////////////////////////////////////////////////////////////////////
// CBoneState: structure describing state of a bone at some point
class CBoneState
{
public:
	// translation of bone relative to root
	CVector3D m_Translation;
	// rotation of bone relative to root
	CQuaternion m_Rotation;
};


////////////////////////////////////////////////////////////////////////////////////////
// CSkeletonAnimDef: raw description - eg bonestates - of an animation that plays upon
// a skeleton
class CSkeletonAnimDef
{
public:
	// current file version given to saved animations
	enum { FILE_VERSION = 1 };
	// supported file read version - files with a version less than this will be rejected
	enum { FILE_READ_VERSION = 1 };


public:
	// Key: description of a single key in a skeleton animation
	typedef CBoneState Key;

public:
	// CSkeletonAnimDef constructor + destructor
	CSkeletonAnimDef();
	~CSkeletonAnimDef();

	// return the number of keys in this animation
	size_t GetNumKeys() const { return (size_t)m_NumKeys; }

	// accessors: get a key for given bone at given time
	Key& GetKey(size_t frame, size_t bone) { return m_Keys[frame*m_NumKeys+bone]; }
	const Key& GetKey(size_t frame, size_t bone) const { return m_Keys[frame*m_NumKeys+bone]; }

	// get duration of this anim, in ms
	float GetDuration() const { return m_NumFrames*m_FrameTime; }

	// return length of each frame, in ms
	float GetFrameTime() const { return m_FrameTime; }
	// return number of frames in animation
	size_t GetNumFrames() const { return (size_t)m_NumFrames; }

	// build matrices for all bones at the given time (in MS) in this animation
	void BuildBoneMatrices(float time, CMatrix3D* matrices, bool loop) const;

	// anim I/O functions
	static CSkeletonAnimDef* Load(const VfsPath& filename);
	static void Save(const VfsPath& pathname, const CSkeletonAnimDef* anim);

public:
	// frame time - time between successive frames, in ms
	float m_FrameTime;
	// number of keys in each frame - should match number of bones in the skeleton
	size_t m_NumKeys;
	// number of frames in the animation
	size_t m_NumFrames;
	// animation data - m_NumKeys*m_NumFrames total keys
	Key* m_Keys;
};

#endif
