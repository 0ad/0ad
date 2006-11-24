
#ifndef H_CinemaTracks_H
#define H_CinemaTracks_H

#include <list>
#include <map>
#include "ps/CStr.h"
#include "maths/NUSpline.h"

/*
	Andrew Decker (aka pyrolink)
	Contact: ajdecker1022@msn.com
	desc: contains various functions used for cinematic camera tracks
	See also: CinemaHandler.cpp, Cinematic.h/.cpp
*/

class CVector3D;
class CVector4D;
class CCamera;

//For loading data
class CCinemaData
{
public:
	CCinemaData() : m_GrowthCount(0), m_Growth(0), m_Switch(0), 
					m_Mode(0), m_Style(0) {}
	virtual ~CCinemaData() {}
	
	const CCinemaData* GetData() const { return this; }
	
	CVector3D m_TotalRotation;
	
	//Distortion variables
	mutable float m_GrowthCount;
	float m_Growth;
	float m_Switch;
	int m_Mode;
	int m_Style;

};

//Once the data is part of the path, it shouldn't be changeable
class CCinemaPath : private CCinemaData, public TNSpline
{
	//friend class CCinemaTrack;
public:
	CCinemaPath(const CCinemaData& data, const TNSpline& spline);
	~CCinemaPath() { DistStylePtr = NULL;  DistModePtr = NULL; }
	
	enum { EM_IN, EM_OUT, EM_INOUT, EM_OUTIN };
	enum { ES_DEFAULT, ES_GROWTH, ES_EXPO, ES_CIRCLE, ES_SINE };
	
	//sets camera position to calculated point on spline
	void MoveToPointAt(float t, const CVector3D &startRotation);
	
	//Distortion mode functions-change how ratio is passed to distortion style functions
	float EaseIn(float t) const;
	float EaseOut(float t) const;
	float EaseInOut(float t) const;
	float EaseOutIn(float t) const;

	//Distortion style functions
	float EaseDefault(float t) const;
	float EaseGrowth(float t) const;
	float EaseExpo(float t) const;
	float EaseCircle(float t) const;
	float EaseSine(float t) const;
	
	float (CCinemaPath::*DistStylePtr)(float ratio) const;
	float (CCinemaPath::*DistModePtr)(float ratio) const;

	const CCinemaData* GetData() const { return CCinemaData::GetData(); }

public:

	void DrawSpline(const CVector4D& RGBA, int smoothness, bool lines) const;

	inline CVector3D GetNodePosition(const int index) const { return Node[index].Position; }
	inline float GetNodeDuration(const int index) const { return Node[index].Distance; }
	inline float GetDuration() const { return MaxDistance; }
	inline float GetElapsedTime() const { return m_TimeElapsed; }
	const std::vector<SplineData>& GetAllNodes() const { return Node; } 
	
	float m_TimeElapsed;
};

class CCinemaTrack
{
	friend class CCinemaManager;
public: 
	CCinemaTrack();
	~CCinemaTrack() {}
	
	void AddPath(const CCinemaData& data, const TNSpline& spline);
	void AddPath(const CCinemaPath& path);
	inline void SetTimescale(float scale) { m_Timescale = scale; }
	inline void SetStartRotation(CVector3D rotation) { m_StartRotation = rotation; }
	void UpdateDuration();

	//Returns false if finished
	bool Play(float DeltaTime);
	bool Validate();

	inline const CVector3D& GetRotation() const { return m_StartRotation; }
	inline float GetTimescale() const { return m_Timescale; }
	inline float GetTotalDuration() const { return m_TotalDuration; }
	inline const std::vector<CCinemaPath>& GetAllPaths() const { return m_Paths; }
	
	
private:
	std::vector<CCinemaPath> m_Paths;
	std::vector<CCinemaPath>::iterator m_CPA;	//current path
	CVector3D m_StartRotation;
	float m_Timescale;	//a negative timescale results in backwards play
	float m_AbsoluteTime;	//Current time of track, in absolute terms (not path)
	float m_TotalDuration;

	bool ValidateForward();
	bool ValidateRewind();
	CVector3D CalculateRotation();
};

//Class for in game playing of cinematics. Should only be instantiated
//in CGameView. 
class CCinemaManager
{
public:
	CCinemaManager();
	~CCinemaManager() {}

	void AddTrack(CCinemaTrack track, const CStrW& name);

	//Adds track to list of being played. 
	void QueueTrack(const CStrW& name, bool queue);
	void OverrideTrack(const CStrW& name);	//clears track queue and replaces with 'name'
	bool Update(float DeltaTime);
	
	//These stop track play, and accept time, not ratio of time
	void MoveToPointAt(float time);
	void MoveToPointAbsolute(float time);	//Time in terms of track

	inline void StopPlaying() { m_TrackQueue.clear(); }
	void DrawAllSplines() const;
	
	inline bool IsPlaying() const { return !m_TrackQueue.empty(); }
	bool HasTrack(const CStrW& name) const; 
	inline bool IsActive() const { return m_Active; }
	inline void SetActive(bool active) { m_Active=active; }

	inline const std::map<CStrW, CCinemaTrack>& GetAllTracks() { return m_Tracks; }
	void SetAllTracks( const std::map<CStrW, CCinemaTrack>& tracks);
	void SetCurrentTrack(const CStrW& name, bool all, bool current, bool lines);
	void SetCurrentPath(int path);

private:
	bool m_Active, m_DrawCurrentSpline, m_DrawAllSplines, m_DrawLines;
	int m_CurrentPath;
	CCinemaTrack* m_CurrentTrack;
	std::map<CStrW, CCinemaTrack> m_Tracks;
	std::list<CCinemaTrack> m_TrackQueue;
};

#endif
