
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

#include "CStr.h"
#include "NUSpline.h"

/*
	Andrew (aka pyrolink)
	Contact: ajdecker1022@msn.com
	desc: contains various functions used for cinematic camera tracks
	Note: There are some differences between the common 'things' of this and the Atlas
	version.  
		The Play functions for both are essentially the same, but Atlas does not support queued tracks.
	The difference between the Track Manager and Cinema Manager is the track manager
	is for adding and deleting tracks to the list, in the editor.  The Cinema Manager is
	for taking care of the queued up Tracks, i.e. when they are to be destroyed/added.
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

	float m_TotalDuration;
	//X=x rotation in degrees...etc
	CVector3D m_TotalRotation;
	
	//Distortion variables
	float m_GrowthCount;
	float m_Growth;
	float m_Switch;
	int m_mode;
	int m_style;

};


class CCinemaPath : public CCinemaData
{
public:
	CCinemaPath(CCinemaData data);
	CCinemaPath() { DistStylePtr = NULL;  DistModePtr = NULL; m_Spline.Init(); }
	~CCinemaPath() { DistStylePtr = NULL;  DistModePtr = NULL; }
	
	float m_TimeElapsed;

	void UpdateSplineEq();
	
	//Resets Rotation-Must call before MoveToPointAt()!!!
	void ResetRotation(float t);
	//sets camera position to calculated point on spline
	void MoveToPointAt(float t);
	
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

public:

	//Used when time and position are already set (usually from loaded file)
	void AddNode(const CVector3D &pos, float timePeriod) { m_Spline.AddNode(pos, timePeriod); }
	void PushNode() { m_Spline.Node.push_back( SplineData() ); }
	void InsertNode(const int index, const CVector3D &pos, float timePeriod) { m_Spline.InsertNode(index, pos, timePeriod); }
	void RemoveNode(const int index) { m_Spline.RemoveNode(index); }
	
	void UpdateNodeTime(const int index, float time) { m_Spline.Node[index].Distance = time; }
    void UpdateNodePosition(const int index, const CVector3D &pos) { m_Spline.Node[index].Position = pos; }
	void DrawSpline(CVector4D RGBA, int smoothness);

	int GetNodeCount() { return m_Spline.NodeCount; }
	CVector3D GetNodePosition(const int index) { return m_Spline.Node[index].Position; }
	float GetDuration() { return m_Spline.MaxDistance; }

	
	//Called when nodes have been added
	void UpdateSpline() { m_Spline.BuildSpline(); }
	void SetSpline( TNSpline spline ) { m_Spline = spline; }
	
private:
	TNSpline m_Spline;

};

class CCinemaTrack
{
public: 
	CCinemaTrack() {}
	~CCinemaTrack() {}
	
	std::vector<CCinemaPath> m_Paths;
	std::vector<CCinemaPath>::iterator m_CPA;	//current path selected
	CVector3D m_StartRotation;
	
	void AddPath(CCinemaData path, TNSpline spline);
	bool Validate();

	//DOES NOT set CPA to Paths.begin().  Returns-false indicates it's finished, 
	//true means it's still playing. 
	bool Play(float DeltaTime);	

};


//Class for in game playing of cinematics
class CCinemaManager
{
public:
	CCinemaManager() {}
	~CCinemaManager() {}

	std::list<CCinemaTrack> m_TrackQueue;
	std::map<CStr, CCinemaTrack> m_Tracks;
	
	int LoadTracks();	//Loads tracks from file
	//Adds track to list of being played.  (Called by triggers?)
	void AddTrack(bool queue, CStr Track);
	bool Update(float DeltaTime);
};

#endif
