
#include "precompiled.h"
#include <string>
#include <sstream>
#include "lib/ogl.h"
#include "CinemaTrack.h"
#include "ps/Game.h"
#include "GameView.h"
#include "maths/MathUtil.h"
#include "Camera.h"
#include "ps/CStr.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "lib/res/file/vfs.h"
#include "lib/res/mem.h"

CCinemaPath::CCinemaPath(const CCinemaData& data, const TNSpline& spline)
: CCinemaData(data), TNSpline(spline)
{ 
	DistStylePtr = &CCinemaPath::EaseDefault;  
	DistModePtr = &CCinemaPath::EaseIn; 
}

void CCinemaPath::DrawSpline(CVector4D RGBA, int smoothness)
{
	if (NodeCount < 3 || DistModePtr == NULL)
		return;
	float start = MaxDistance / smoothness;
	CVector3D tmp;
	float time=0;

	glColor4f( RGBA.m_X, RGBA.m_Y, RGBA.m_Z, RGBA.m_W );
	glLineWidth(2.0f);
	glBegin(GL_LINE_STRIP);
	for (int i=0; i<smoothness; i++)
	{
		//Find distorted time
		time = (this->*DistModePtr)(start*i / MaxDistance);
		tmp = GetPosition(time);
		glVertex3f( tmp.X, tmp.Y, tmp.Z );
	}
	glEnd();
	glLineWidth(1.0f);

	glPointSize(6.0f);
	//Draw spline endpoints
	glBegin(GL_POINTS);
		tmp = GetPosition(0);
		glVertex3f( tmp.X, tmp.Y, tmp.Z );
		tmp = GetPosition(1);
		glVertex3f( tmp.X, tmp.Y, tmp.Z );
	glEnd();
	glPointSize(1.0f);
}
void CCinemaPath::MoveToPointAt(float t, const CVector3D &startRotation)
{
	CCamera *Cam=g_Game->GetView()->GetCamera();
	t = (this->*DistModePtr)(t);
	CVector3D rot = startRotation + m_TotalRotation * t;
	Cam->m_Orientation.SetXRotation(fmodf(rot.X, 360.0f));
	Cam->m_Orientation.RotateY(fmodf(rot.Y, 360.0f));
	Cam->m_Orientation.RotateZ(fmodf(rot.Z, 360.0f));

	CVector3D pos = GetPosition(t);
	Cam->m_Orientation.Translate(pos);
}


void CCinemaPath::ResetRotation(float t)
{
	CCamera *Cam=g_Game->GetView()->GetCamera();
	t = (this->*DistModePtr)(t);
	CVector3D rot = m_TotalRotation * t;

	//rotate in reverse order and reverse angles
	Cam->m_Orientation.RotateZ(-fmodf(rot.Z, 360));
	Cam->m_Orientation.RotateY(-fmodf(rot.Y, 360));
	Cam->m_Orientation.RotateX(-fmodf(rot.X, 360));
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
void CCinemaTrack::AddPath(CCinemaData& data, TNSpline& spline)
{
	CCinemaPath path(data, spline);
	path.m_TimeElapsed=0;

	path.BuildSpline();
	m_Paths.push_back(path);
	std::vector<CCinemaPath>::iterator SetTemp;
	SetTemp=m_Paths.end() - 1;

	//Set distortion mode and style
	switch(data.m_Mode)
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
		debug_printf("Cinematic mode not found for %d ", data.m_Mode);
		break;
	}

	switch (data.m_Style)
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
		debug_printf("Cinematic mode not found for %d !", data.m_Style);
		break;
	}
	UpdateDuration();

}
void CCinemaTrack::UpdateDuration()
{
	m_TotalDuration=0;
	for ( std::vector<CCinemaPath>::iterator it=m_Paths.begin(); it!=m_Paths.end(); it++ )
	{
		m_TotalDuration += it->MaxDistance;
	}
}
bool CCinemaTrack::Validate()
{
	if ( m_CPA->m_TimeElapsed > 0.f )
		return ValidateForward();
	else
		return ValidateRewind();
}

bool CCinemaTrack::ValidateRewind()
{
	if (m_CPA->m_TimeElapsed < 0)
	{
		if (m_CPA == m_Paths.begin())			
		{
			return false;
		}
		//Make sure it's within limits of path
		else
		{
			float Pos=m_CPA->m_TimeElapsed;
			m_CPA--;	
			while (1)
			{
				if (m_CPA->GetDuration() + Pos < 0)
				{
					if (m_CPA == m_Paths.begin())
					{
						 m_CPA->m_TimeElapsed = m_CPA->MaxDistance;
						 m_CPA->MoveToPointAt(0.0f, m_StartRotation );
						 return false;
					}
						Pos+=m_CPA->GetDuration();
						m_CPA->m_TimeElapsed=0;
						m_CPA--; 		
				}		
				else
				{
					m_CPA->m_TimeElapsed+=Pos;
					break;
				}
			}
		}	//inside limits
	}	
	return true;
}

bool CCinemaTrack::ValidateForward()
{
	if (m_CPA->m_TimeElapsed >= m_CPA->MaxDistance)
	{
		if (m_CPA == m_Paths.end() - 1)			
		{
			return false;
		}
		//Make sure it's within limits of path
		else
		{
			float Pos = m_CPA->m_TimeElapsed - m_CPA->MaxDistance; 
			m_CPA++;	

			while (1)
			{
				if (Pos > m_CPA->MaxDistance)
				{
					if (m_CPA == m_Paths.end() -1)
					{
						 m_CPA->m_TimeElapsed = m_CPA->MaxDistance;
						 m_CPA->MoveToPointAt(1.0f, m_StartRotation );
						 return false;
					}
						Pos -= m_CPA->MaxDistance;
						m_CPA->m_TimeElapsed = m_CPA->MaxDistance;
						m_CPA++; 		
				}		
				else
				{
					m_CPA->m_TimeElapsed+=Pos;
					break;
				}
			}
		}	//inside limits of path
	}	
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
			 Cam->SetProjection (1, 5000, DEGTORAD(20));
			 Cam->m_Orientation.SetXRotation(m_StartRotation.X);
			 Cam->m_Orientation.RotateY(m_StartRotation.Y);
			 Cam->m_Orientation.RotateZ(m_StartRotation.Z);

		 }
	}
	m_CPA->m_TimeElapsed += m_TimeScale*DeltaTime;
	m_AbsoluteTime += m_TimeScale*DeltaTime;

	if (!Validate())
		return false;
	m_CPA->MoveToPointAt( m_CPA->GetElapsedTime() / m_CPA->GetDuration(), m_StartRotation);
	return true;
}
void CCinemaTrack::MoveToPointAt(float t)
{
	m_CPA->m_TimeElapsed = t;
	if ( !Validate() )
		return;
	MoveToPointAt(m_CPA->m_TimeElapsed / m_CPA->GetDuration());
}
void CCinemaTrack::MoveToPointAbsolute(float time)
{
	m_CPA = m_Paths.begin();
	m_AbsoluteTime = m_CPA->m_TimeElapsed = time;
	
	if (!ValidateForward())
		return;
	else
		m_CPA->MoveToPointAt(m_CPA->m_TimeElapsed, m_StartRotation);
}

void CCinemaManager::AddTrack(CCinemaTrack track, CStrW name)
{
	debug_assert( m_Tracks.find( name ) == m_Tracks.end() );
	m_Tracks[name] = track;
}

void CCinemaManager::QueueTrack(CStrW name, bool queue )
{
	if (!m_TrackQueue.empty() && queue == false)
	{
		return;
	}
	else
	{
		debug_assert(HasTrack(name));
		m_TrackQueue.push_back(m_Tracks[name]);
		m_TrackQueue.back().m_CPA = m_TrackQueue.back().m_Paths.begin();
	}
}
void CCinemaManager::OverrideTrack(CStrW name)
{
	m_TrackQueue.clear();
	debug_assert(HasTrack(name));
	m_TrackQueue.push_back( m_Tracks[name] );
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
	FileIOBuf buf; size_t size;
	RETURN_ERR(vfs_load("FILENAME", buf, size));
	std::istringstream Stream(std::string((const char*)buf, (int)size), std::istringstream::binary);
	(void)file_buf_free(buf);

	//Read in lone data
	Stream >> fileID;
	Stream >> numTracks;

	if (fileID != 0x0ADC)
	{
		debug_printf("Cinematic file not found for (FILENAME)!");
		return 0;
	}
	for (int i=0; i < numTracks; i++)
	{
		CCinemaTrack tmpTrack;

		CVector3D tmpPos;
		float tmpTime;
		int numPaths;
		CStr Name;

		Stream >> Name;
		Stream >> numPaths;
		Stream >> tmpTrack.m_StartRotation.X;
		Stream >> tmpTrack.m_StartRotation.Y;
		Stream >> tmpTrack.m_StartRotation.Z;

		for (int j=0; j < numPaths; j++)
		{
			CCinemaData tmpData;
			TNSpline tmpSpline;
			int numNodes;
			//load main data
			Stream >> numNodes;
			Stream >> tmpData.m_TotalRotation.X;
			Stream >> tmpData.m_TotalRotation.Y;
			Stream >> tmpData.m_TotalRotation.Z;
			Stream >> tmpData.m_Growth;
			tmpData.m_GrowthCount = tmpData.m_Growth;
			Stream >> tmpData.m_Switch;
			Stream >> tmpData.m_Mode;
			Stream >> tmpData.m_Style;

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
		AddTrack(tmpTrack, Name);
	}
	return 0;
}