///////////////////////////////////////////////////////////////////////////////
//
// Name:		SkeletonAnimDef.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SKELETONANIMDEF_H
#define _SKELETONANIMDEF_H

#include "res/res.h"
#include "CStr.h"
#include "Vector3D.h"
#include "Quaternion.h"

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
	u32 GetNumKeys() const { return m_NumKeys; }

	// accessors: get a key for given bone at given time
	Key& GetKey(u32 frame,u32 bone) { return m_Keys[frame*m_NumKeys+bone]; }
	const Key& GetKey(u32 frame,u32 bone) const { return m_Keys[frame*m_NumKeys+bone]; }	

	// get duration of this anim, in ms
	float GetDuration() const { return m_NumFrames*m_FrameTime; }

	// return length of each frame, in ms
	float GetFrameTime() const { return m_FrameTime; }
	// return number of frames in animation
	u32 GetNumFrames() const { return m_NumFrames; }

	// build matrices for all bones at the given time (in MS) in this animation
	void BuildBoneMatrices(float time,CMatrix3D* matrices) const;

	// anim I/O functions
	static CSkeletonAnimDef* Load(const char* filename);
	static void Save(const char* filename,const CSkeletonAnimDef* anim);

public:
	// name of the animation
	CStr m_Name;
	// frame time - time between successive frames, in ms
	float m_FrameTime;
	// number of keys in each frame - should match number of bones in the skeleton
	u32 m_NumKeys;
	// number of frames in the animation
	u32 m_NumFrames;
	// animation data - m_NumKeys*m_NumFrames total keys
	Key* m_Keys;
};

#endif