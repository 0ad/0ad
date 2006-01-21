
#include "precompiled.h"
#include <string>
#include <sstream>
#include "ogl.h"
#include "CinemaTrack.h"
#include "Game.h"
#include "GameView.h"
#include "MathUtil.h"
#include "Camera.h"
#include "CStr.h"
#include "Vector3D.h"
#include "Vector4D.h"
#include "lib/res/file/vfs.h"
#include "lib/res/mem.h"

CCinemaPath::CCinemaPath(CCinemaData data)
{
	m_TotalDuration = data.m_TotalDuration;
	m_TotalRotation = data.m_TotalRotation;
	m_GrowthCount = data.m_GrowthCount;
	m_Growth = data.m_Growth;
	m_Switch = data.m_Switch;
}

void CCinemaPath::DrawSpline(CVector4D RGBA, int smoothness)
{
	float start = m_Spline.MaxDistance / smoothness;
	CVector3D tmp;
	float time=0;

	glColor4f( RGBA.m_X, RGBA.m_Y, RGBA.m_Z, RGBA.m_W );
	glPointSize(4.0);
	
	
	glBegin(GL_LINE_STRIP);
	tmp = m_Spline.GetPosition(0);
	glVertex3f( tmp.X, tmp.Y, tmp.Z );
	for (int i=0; i<smoothness; i++)
	{
		//Find distorted time
		time = (this->*DistModePtr)(start*1);
		tmp = m_Spline.GetPosition(time);
		glVertex3f( tmp.X, tmp.Y, tmp.Z );
	}
	glEnd();
	
	//Draw spline endpoints
	glBegin(GL_POINT);
		tmp = m_Spline.GetPosition(0);
		glVertex3f( tmp.X, tmp.Y, tmp.Z );
		tmp = m_Spline.GetPosition(1);
		glVertex3f( tmp.X, tmp.Y, tmp.Z );
	glEnd();
}
void CCinemaPath::MoveToPointAt(float t)
{
	CCamera *Cam=g_Game->GetView()->GetCamera();
	t = (this->*DistModePtr)(t);
	CVector3D pos = m_Spline.GetPosition(t);
	Cam->m_Orientation.SetTranslation(pos);
	CVector3D rot = m_TotalRotation * t;
	
	Cam->m_Orientation.RotateX(DEGTORAD(fmodf(rot.X, 360)));
	Cam->m_Orientation.RotateY(DEGTORAD(fmodf(rot.Y, 360)));
	Cam->m_Orientation.RotateZ(DEGTORAD(fmodf(rot.Z, 360)));
}


void CCinemaPath::ResetRotation(float t)
{
	CCamera *Cam=g_Game->GetView()->GetCamera();
	t = (this->*DistModePtr)(t);
	CVector3D rot = m_TotalRotation * t;
	
	//rotate in reverse order and reverse angles
	Cam->m_Orientation.RotateZ(DEGTORAD(-fmodf(rot.Z, 360)));
	Cam->m_Orientation.RotateY(DEGTORAD(-fmodf(rot.Y, 360)));
	Cam->m_Orientation.RotateX(DEGTORAD(-fmodf(rot.X, 360)));
	
}


//Distortion mode functions
float CCinemaPath::EaseIn(float t)
{
	return (this->*DistStylePtr)(t);
}
float CCinemaPath::EaseOut(float t)
{
	return 1.0f - EaseIn(1.0f-t);
}
float CCinemaPath::EaseInOut(float t)
{
	if (t < m_Switch)
		return EaseIn(1.0f/m_Switch * t) * m_Switch;
	return EaseOut(1.0f/m_Switch * (t-m_Switch)) * m_Switch + m_Switch;
}
float CCinemaPath::EaseOutIn(float t)
{
	if (t < m_Switch)
		return EaseOut(1.0f/m_Switch * t) * m_Switch;
	return EaseIn(1.0f/m_Switch * (t-m_Switch)) * m_Switch + m_Switch;
}


//Distortion style functions
float CCinemaPath::EaseDefault(float t)
{
	return t;
}
float CCinemaPath::EaseGrowth(float t)
{
	return pow(t, m_Growth);
}

float CCinemaPath::EaseExpo(float t)
{
	if(t == 0) 
		return t;
    return powf(m_Growth, 10*(t-1.0f));
}
float CCinemaPath::EaseCircle(float t)
{
	 t = -(sqrt(1.0f - t*t) - 1.0f);
     if(m_GrowthCount > 1.0f) 
	 {
		 m_GrowthCount--;
		 return (this->*DistStylePtr)(t);
	 }
	 return t;
}

float CCinemaPath::EaseSine(float t)
{
     t = 1.0f - cos(t * PI/2);
     if(m_GrowthCount > 1.0f) 
	 {
		 m_GrowthCount--;
		 return (this->*DistStylePtr)(t);
	 }
	 return t;
}


//-------CinemaTrack functions------
//AddPath-For building tracks from loaded file
void CCinemaTrack::AddPath(CCinemaData data, TNSpline spline)
{
	CCinemaPath path(data);
	path.m_TimeElapsed=0;
	path.SetSpline(spline);

	path.UpdateSpline();
	m_Paths.push_back(path);
	std::vector<CCinemaPath>::iterator SetTemp;
	SetTemp=m_Paths.end() - 1;
	
	//Set distortion mode and style
	switch(data.m_mode)
	{
	case EM_IN:	
		SetTemp->DistModePtr = &CCinemaPath::EaseIn;
		break;
	case EM_OUT: 
		SetTemp->DistModePtr = &CCinemaPath::EaseOut;
		break;
	case EM_INOUT:	
		SetTemp->DistModePtr = &CCinemaPath::EaseInOut;
		break;
	case EM_OUTIN: 
		SetTemp->DistModePtr = &CCinemaPath::EaseOutIn;
		break;
	default:
		debug_printf("Cinematic mode not found for %d ", data.m_mode);
		break;
	}
	
	switch (data.m_style)
	{
	case ES_DEFAULT:
		SetTemp->DistStylePtr = &CCinemaPath::EaseDefault;
		break;
	case ES_GROWTH:
		SetTemp->DistStylePtr = &CCinemaPath::EaseGrowth;
		break;
	case ES_EXPO:
		SetTemp->DistStylePtr = &CCinemaPath::EaseExpo;
		break;
	case ES_CIRCLE:
		SetTemp->DistStylePtr = &CCinemaPath::EaseCircle;
		break;
	case ES_SINE: 
		SetTemp->DistStylePtr = &CCinemaPath::EaseSine;
		break;
	default:
		debug_printf("Cinematic mode not found for %d !", data.m_style);
		break;
	}

	
}
bool CCinemaTrack::Validate()
{	
	if (m_CPA->m_TimeElapsed >= m_CPA->m_TotalDuration)
	{
		if (m_CPA == m_Paths.end() - 1)			
		{
			return false;
		}
		//Make sure it's within limits of path
		else
		{
			float Pos = m_CPA->m_TimeElapsed - m_CPA->m_TotalDuration; 
			m_CPA++;

			while (1)
			{
				if (Pos > m_CPA->m_TotalDuration)
				{
					if (m_CPA == m_Paths.end() -1)
					{
						 return false;
					}
						Pos -= m_CPA->m_TotalDuration;
						m_CPA++; 		
				}		
				else
				{
					m_CPA->m_TimeElapsed+=Pos;
					break;
				}
			}	//while
		}	
    }	//main if statement
   return true;
}

bool CCinemaTrack::Play(float DeltaTime)
{
	if (m_CPA->m_TimeElapsed == 0)
	{
		 if (m_CPA == m_Paths.begin())
		 {
			 //Set camera to start at starting rotations
			 CCamera *Cam=g_Game->GetView()->GetCamera();
			 Cam->m_Orientation.SetIdentity();
			 Cam->m_Orientation.SetXRotation(DEGTORAD(m_StartRotation.X));
			 Cam->m_Orientation.RotateY(DEGTORAD(m_StartRotation.Y));
			 Cam->m_Orientation.RotateZ(DEGTORAD(m_StartRotation.Z));
			 Cam->SetProjection (1, 5000, DEGTORAD(20));
		 }
	}
	m_CPA->ResetRotation(m_CPA->m_TimeElapsed / m_CPA->m_TotalDuration);
	m_CPA->m_TimeElapsed += DeltaTime;

	if (!Validate())
	{
		m_CPA->MoveToPointAt(1);
		return false;
	}
	m_CPA->MoveToPointAt(m_CPA->m_TimeElapsed / m_CPA->m_TotalDuration);
	return true;
}

void CCinemaManager::AddTrack(bool queue, CStr Track)
{
	if (!m_TrackQueue.empty() && queue == false)
	{
		return;
	}
	else
	{
		m_TrackQueue.push_back(m_Tracks[Track]); 
		m_TrackQueue.back().m_CPA = m_TrackQueue.back().m_Paths.begin();
	}
}
bool CCinemaManager::Update(float DeltaTime)
{
	if (!m_TrackQueue.front().Play(DeltaTime))
	{
		m_TrackQueue.pop_front();
		return false;
	}
	return true;
}

int CCinemaManager::LoadTracks()
{
	unsigned int fileID;
	int numTracks;
	
	//NOTE: How do you find the current scenario's cinematics?
	void* fData;
	size_t fSize;
	Handle hm = vfs_load("FILENAME", fData, fSize);
	RETURN_ERR(hm);
	std::istringstream Stream(std::string((const char*)fData, (int)fSize), std::istringstream::binary);
	
	//Read in lone data
	Stream >> fileID;
	Stream >> numTracks;
	
	if (fileID != 0x0ADC)
	{
		debug_printf("Cinematic file not found for (FILENAME)!");
		mem_free_h(hm);
		return 0;
	}
	for (int i=0; i < numTracks; i++)
	{
		CCinemaTrack tmpTrack;
		
		CVector3D tmpPos;
		float tmpTime;
		int numPaths;
		int numNodes;
		CStr Name;
		
		Stream >> Name;
		Stream >> numPaths;
		Stream >> numNodes;
		Stream >> tmpTrack.m_StartRotation.X;
		Stream >> tmpTrack.m_StartRotation.Y;
		Stream >> tmpTrack.m_StartRotation.Z;
		
		for (int j=0; j < numPaths; j++)
		{
			CCinemaData tmpData;
			TNSpline tmpSpline;
			//load main data
			Stream >> tmpData.m_TotalDuration;
			Stream >> tmpData.m_TotalRotation.X;
			Stream >> tmpData.m_TotalRotation.Y;
			Stream >> tmpData.m_TotalRotation.Z;
			Stream >> tmpData.m_Growth;
			tmpData.m_GrowthCount = tmpData.m_Growth;
			Stream >> tmpData.m_Switch;
			Stream >> tmpData.m_mode;
			Stream >> tmpData.m_style;

			//Get point data for path
			for (int x=0; x<numNodes; x++)
			{
				Stream >> tmpPos.X;
				Stream >> tmpPos.Y;
				Stream >> tmpPos.Z;
				Stream >> tmpTime;
				tmpSpline.AddNode( tmpPos, tmpTime ); 
			}
			tmpTrack.AddPath(tmpData, tmpSpline);
		}
		m_Tracks[Name] = tmpTrack;
	}
	mem_free_h(hm);
	return 0;
}


