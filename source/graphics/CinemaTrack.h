
#ifndef H_CinemaTracks_H
#define H_CinemaTracks_H

#define EM_IN 0
#define EM_OUT 1
#define EM_INOUT 2
#define EM_OUTIN 3

#define ES_DEFAULT 0
#define ES_GROWTH 1
#define ES_EXPO 2
#define ES_CIRCLE 3
#define ES_SINE 4

#include <list>
#include <map>
#include "ps/CStr.h"
#include "maths/NUSpline.h"

/*
	Andrew (aka pyrolink)
	Contact: ajdecker1022@msn.com
	desc: contains various functions used for cinematic camera tracks
*/

class CVector3D;
class CVector4D;
class CCamera;

//For loading data
class CCinemaData
{
public:
	CCinemaData() {}
	~CCinemaData() {}
	
	const CCinemaData* GetData() const { return this; }
	
	CVector3D m_TotalRotation;
	
	//Distortion variables
	float m_GrowthCount;
	float m_Growth;
	float m_Switch;
	int m_Mode;
	int m_Style;

};

//Once the data is part of the path, it shouldn't be changeable
class CCinemaPath : private CCinemaData, public TNSpline
{
	friend class CCinemaTrack;
public:
	CCinemaPath(const CCinemaData& data, const TNSpline& spline);
	~CCinemaPath() { DistStylePtr = NULL;  DistModePtr = NULL; }
	
	void ResetRotation(float t);
	//sets camera position to calculated point on spline
	void MoveToPointAt(float t, const CVector3D &startRotation);
	
	//void MoveToPointAt(float t);
	
	//Distortion mode functions-change how ratio is passed to distortion style functions
	float EaseIn(float t);
	float EaseOut(float t);
	float EaseInOut(float t);
	float EaseOutIn(float t);

	//Distortion style functions
	float EaseDefault(float t);
	float EaseGrowth(float t);
	float EaseExpo(float t);
	float EaseCircle(float t);
	float EaseSine(float t);
	
	float (CCinemaPath::*DistStylePtr)(float ratio);
	float (CCinemaPath::*DistModePtr)(float ratio);

	const CCinemaData* GetData() const { return CCinemaData::GetData(); }

public:

	void DrawSpline(CVector4D RGBA, int smoothness);

	inline CVector3D GetNodePosition(const int index) const { return Node[index].Position; }
	inline float GetNodeDuration(const int index) const { return Node[index].Distance; }
	inline float GetDuration() const { return MaxDistance; }
	inline float GetElapsedTime() const { return m_TimeElapsed; }
	const std::vector<SplineData>& GetAllNodes() const { return Node; } 
//	inline void SetElapsedTime(float time) { m_TimeElapsed = time; }
	
private:
	float m_TimeElapsed;

};

class CCinemaTrack
{
	friend class CCinemaManager;
public: 
	CCinemaTrack() {}
	~CCinemaTrack() {}
	
	void AddPath(CCinemaData& data, TNSpline& spline);
	inline void SetTimeScale(float scale) { m_TimeScale = scale; }
	inline void SetStartRotation(CVector3D rotation) { m_StartRotation = rotation; }
	void UpdateDuration();

	//DOES NOT set CPA to Paths.begin().  Returns-false indicates it's finished, 
	//true means it's still playing. 
	bool Play(float DeltaTime);
	bool Validate();
	void MoveToPointAt(float t);
	void MoveToPointAbsolute(float time);	//Time, not ratio, in terms of track

	inline const CVector3D& GetRotation() const { return m_StartRotation; }
	inline float GetTimeScale() const { return m_TimeScale; }
	inline float GetTotalDuration() const { return m_TotalDuration; }
	inline const std::vector<CCinemaPath>& GetAllPaths() const { return m_Paths; }

private:
	std::vector<CCinemaPath> m_Paths;
	std::vector<CCinemaPath>::iterator m_CPA;	//current path
	CVector3D m_StartRotation;
	float m_TimeScale;	//a negative timescale results in backwards play
	float m_AbsoluteTime;	//Current time of track, in absolute terms (not path)
	float m_TotalDuration;

	bool ValidateForward();
	bool ValidateRewind();
};

//Class for in game playing of cinematics
class CCinemaManager
{
public:
	CCinemaManager() { m_Active=false; }
	~CCinemaManager() {}
	
	void AddTrack(CCinemaTrack track, CStrW name);
	int LoadTracks();	//Loads tracks from file
	
	//Adds track to list of being played.  (Called by triggers?)
	void QueueTrack(CStrW name, bool queue);
	void OverrideTrack(CStrW name);	//clears track queue and replaces with 'name'
	bool Update(float DeltaTime);
	
	inline bool IsPlaying() const { return !m_TrackQueue.empty(); }
	bool HasTrack(CStrW name) const { return m_Tracks.find(name) != m_Tracks.end(); }
	inline bool IsActive() const { return m_Active; }
	inline void SetActive(bool active) { m_Active=active; }

	CCinemaTrack* GetTrack(CStrW name) { debug_assert(HasTrack(name)); return &m_Tracks[name]; } 
	inline const std::map<CStrW, CCinemaTrack>& GetAllTracks() { return m_Tracks; }
	inline void SetAllTracks( const std::map<CStrW, CCinemaTrack>& tracks) { m_Tracks = tracks; }
private:
	bool m_Active;
	std::map<CStrW, CCinemaTrack> m_Tracks;
	std::list<CCinemaTrack> m_TrackQueue;
};

#endif
