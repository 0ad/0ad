
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
	if (GetNodeCount() < 3 || DistModePtr == NULL)
		return;
	float start = m_Spline.MaxDistance / smoothness;
	CVector3D tmp;
	float time=0;

	glColor4f( RGBA.m_X, RGBA.m_Y, RGBA.m_Z, RGBA.m_W );
	glLineWidth(2.0f);
	glBegin(GL_LINE_STRIP);
	for (int i=0; i<smoothness; i++)
	{
		//Find distorted time
		time = (this->*DistModePtr)(start*i / m_Spline.MaxDistance);
		tmp = m_Spline.GetPosition(time);
		glVertex3f( tmp.X, tmp.Y, tmp.Z );
	}
	glEnd();
	glLineWidth(1.0f);

	glPointSize(6.0f);
	//Draw spline endpoints
	glBegin(GL_POINTS);
		tmp = m_Spline.GetPosition(0);
		glVertex3f( tmp.X, tmp.Y, tmp.Z );
		tmp = m_Spline.GetPosition(1);
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

	CVector3D pos = m_Spline.GetPosition(t);
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
void CCinemaTrack::AddPath(CCinemaData data, TNSpline &spline)
{
	CCinemaPath path(data);
	path.m_TimeElapsed=0;
	path.SetSpline(spline);
	path.m_TotalDuration = path.GetDuration();

	path.UpdateSpline();
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


}
bool CCinemaTrack::Validate()
{
	if (m_CPA->m_TimeElapsed >= m_CPA->m_TotalDuration)
	{
		if (m_CPA == m_Paths.end() - 1)
		{
			return false;
		}
		//Make sure it's within limits of track
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
			 Cam->SetProjection (1, 5000, DEGTORAD(20));
			 Cam->m_Orientation.SetXRotation(m_StartRotation.X);
			 Cam->m_Orientation.RotateY(m_StartRotation.Y);
			 Cam->m_Orientation.RotateZ(m_StartRotation.Z);

		 }
	}
	//m_CPA->ResetRotation(m_CPA->m_TimeElapsed / m_CPA->m_TotalDuration);
	m_CPA->m_TimeElapsed += DeltaTime;

	if (!Validate())
	{
		m_CPA->MoveToPointAt(1, m_StartRotation);
		return false;
	}
	m_CPA->MoveToPointAt(m_CPA->m_TimeElapsed / m_CPA->m_TotalDuration, m_StartRotation);
	return true;
}
void CCinemaManager::AddTrack(CCinemaTrack track, CStr name)
{
	debug_assert( m_Tracks.find( name ) == m_Tracks.end() );
	m_Tracks[name] = track;
}

void CCinemaManager::QueueTrack(CStr name, bool queue )
{
	if (!m_TrackQueue.empty() && queue == false)
	{
		return;
	}
	else
	{
		m_TrackQueue.push_back(m_Tracks[name]);
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
bool CCinemaManager::IsPlaying()
{
	return !m_TrackQueue.empty();
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
			Stream >> tmpData.m_TotalDuration;
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
int CCinemaManager::HACK_WriteTrack(CCinemaTrack track)
{
	char *name = "cinematic.cnm";
	FILE *fp = fopen(name, "wb");
	unsigned int fileID = 0x0ADC;
	int numTracks = 1;

	int numPaths = track.m_Paths.size();

	fwrite( &fileID, sizeof(fileID), 1, fp);
	fwrite( &numTracks, sizeof(numTracks), 1, fp);
	fwrite( &name, sizeof(name), 1, fp);
	fwrite( &numPaths, sizeof(numPaths), 1, fp);
	fwrite( &track.m_StartRotation, sizeof(track.m_StartRotation), 1, fp );

	for (int i=0; i<numPaths; i++)
	{
		int count = track.m_Paths[i].GetNodeCount();
		float duration = track.m_Paths[i].GetDuration();

		fwrite( &count, sizeof(int), 1, fp );
		fwrite( &duration, sizeof(float), 1, fp );
		fwrite( &track.m_Paths[i].m_TotalRotation, sizeof(CVector3D), 1, fp );
		fwrite( &track.m_Paths[i].m_Growth, sizeof(float), 1, fp );
		fwrite( &track.m_Paths[i].m_Switch, sizeof(float), 1, fp );
		fwrite( &track.m_Paths[i].m_Growth, sizeof(float), 1, fp );
		fwrite( &track.m_Paths[i].m_Mode, sizeof(float), 1, fp );
		fwrite( &track.m_Paths[i].m_Style, sizeof(float), 1, fp );

		for (int j=0; j < track.m_Paths[i].GetNodeCount(); j++)
		{
			CVector3D position = track.m_Paths[i].GetNodePosition(j);
			float duration = track.m_Paths[i].GetNodeDuration(j);
			fwrite( &position, sizeof(CVector3D), 1, fp );
			fwrite( &duration, sizeof(float), 1, fp );
		}
	}
	fclose(fp);

	return 0;
}

