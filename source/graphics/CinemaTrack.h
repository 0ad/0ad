
#ifndef H_CinemaTracks_H
#define H_CinemaTracks_H

#define EM_IN 0
#define EM_OUT 1
#define EM_INOUT 2
#define EM_OUTIN 3

#define ES_DEFAULT 0
#define ES_QUAD 1
#define ES_EXPO 2
#define ES_CIRCLE 3
#define ES_SINE 4

#include <stdlib.h>
#include <list>
#include <map>
#include "Camera.h"
#include "CStr.h"
#include "Vector3D.h"

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



//For loading data
class CCinemaData
{
public:	
	CCinemaData() {}
	~CCinemaData() {}

	float m_TotalDuration;
	//X=x rotation in degrees...etc
	CVector3D m_TotalRotation;
	CVector3D m_StartRotation;
	
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
	CCinemaPath() { DistStylePtr = NULL;  DistModePtr = NULL; }
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
	float EaseQuad(float t);
	float EaseExpo(float t);
	float EaseCircle(float t);
	float EaseSine(float t);
	
	float (CCinemaPath::*DistStylePtr)(float ratio);
	float (CCinemaPath::*DistModePtr)(float ratio);

	//returns point on spline
	CVector3D RetrievePointAt(float t);
	CVector3D getPoint(int point){ return m_Points[point]; }
	void SetPoint(int point, CVector3D value) { m_Points[point]=value; }
	
private:
	CVector3D m_Points[4];
	//coefficents used in equation
	float Ax, Bx, Cx, Ay, By, Cy, Az, Bz, Cz;  

};

class CCinemaTrack
{
public: 
	CCinemaTrack() {}
	~CCinemaTrack() {}

	std::vector<CCinemaPath> m_Paths;
	std::vector<CCinemaPath>::iterator m_CPA;	//current path selected (in listbox?)
	
	void AddPath(CCinemaData path, CVector3D points[4]);
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
