
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
: CCinemaData(data), TNSpline(spline), m_TimeElapsed(0.f)
{ 
	DistStylePtr = &CCinemaPath::EaseDefault;  
	DistModePtr = &CCinemaPath::EaseIn; 
}

void CCinemaPath::DrawSpline(const CVector4D& RGBA, int smoothness, bool lines) const
{
	if (NodeCount < 2 || DistModePtr == NULL)
		return;
	if ( NodeCount == 2 )
		smoothness = 2;

	float start = MaxDistance / smoothness;
	float time=0;
	
	glColor4f( RGBA.m_X, RGBA.m_Y, RGBA.m_Z, RGBA.m_W );
	if ( lines )
	{	
		glLineWidth(1.8f);
		glEnable(GL_LINE_SMOOTH);
		glBegin(GL_LINE_STRIP);

		for (int i=0; i<smoothness; ++i)
		{
			//Find distorted time
			time = start*i / MaxDistance;
			CVector3D tmp = GetPosition(time);
			glVertex3f( tmp.X, tmp.Y, tmp.Z );
		}
		glEnd();
		glDisable(GL_LINE_SMOOTH);
		glLineWidth(1.0f);
	}
	else
	{
		smoothness /= 2;
		start = MaxDistance / smoothness;
		glEnable(GL_POINT_SMOOTH);
		glPointSize(3.0f);
		glBegin(GL_POINTS);

		for (int i=0; i<smoothness; ++i)
		{
			//Find distorted time
			time = (this->*DistModePtr)(start*i / MaxDistance);
			CVector3D tmp = GetPosition(time);
			glVertex3f( tmp.X, tmp.Y, tmp.Z );
		}
		glColor3f(1.0f, 1.0f, 0.0f);	//yellow

		for ( size_t i=0; i<Node.size(); ++i )
			glVertex3f(Node[i].Position.X, Node[i].Position.Y, Node[i].Position.Z);

		glEnd();
		glPointSize(1.0f);
		glDisable(GL_POINT_SMOOTH);
	}
}
void CCinemaPath::MoveToPointAt(float t, const CVector3D &startRotation)
{
	CCamera *Cam=g_Game->GetView()->GetCamera();
	t = (this->*DistModePtr)(t);
	CVector3D rot = startRotation + m_TotalRotation * t;
	Cam->m_Orientation.SetXRotation(fmodf(rot.X, DEGTORAD(360.0f)) );
	Cam->m_Orientation.RotateY( fmodf(rot.Y, DEGTORAD(360.0f)) );
	Cam->m_Orientation.RotateZ( fmodf(rot.Z, DEGTORAD(360.0f)) );

	CVector3D pos = GetPosition(t);
	Cam->m_Orientation.Translate(pos);
	Cam->UpdateFrustum();
}

//Distortion mode functions
float CCinemaPath::EaseIn(float t) const
{
	return (this->*DistStylePtr)(t);
}
float CCinemaPath::EaseOut(float t) const
{
	return 1.0f - EaseIn(1.0f-t);
}
float CCinemaPath::EaseInOut(float t) const
{
	if (t < m_Switch)
		return EaseIn(1.0f/m_Switch * t) * m_Switch;
	return EaseOut(1.0f/m_Switch * (t-m_Switch)) * m_Switch + m_Switch;
}
float CCinemaPath::EaseOutIn(float t) const
{
	if (t < m_Switch)
		return EaseOut(1.0f/m_Switch * t) * m_Switch;
	return EaseIn(1.0f/m_Switch * (t-m_Switch)) * m_Switch + m_Switch;
}


//Distortion style functions
float CCinemaPath::EaseDefault(float t) const
{
	return t;
}
float CCinemaPath::EaseGrowth(float t) const
{
	return pow(t, m_Growth);
}

float CCinemaPath::EaseExpo(float t) const
{
	if(t == 0)
		return t;
    return powf(m_Growth, 10*(t-1.0f));
}
float CCinemaPath::EaseCircle(float t) const
{
	 t = -(sqrt(1.0f - t*t) - 1.0f);
     if(m_GrowthCount > 1.0f)
	 {
		 m_GrowthCount--;
		 return (this->*DistStylePtr)(t);
	 }
	 return t;
}

float CCinemaPath::EaseSine(float t) const
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
CCinemaTrack::CCinemaTrack() : m_AbsoluteTime(0), m_Timescale(1),
	m_TotalDuration(0)
{
	 m_CPA = m_Paths.end(); 
}
void CCinemaTrack::AddPath(const CCinemaData& data, const TNSpline& spline)
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
	case CCinemaPath::EM_IN:
		SetTemp->DistModePtr = &CCinemaPath::EaseIn;
		break;
	case CCinemaPath::EM_OUT:
		SetTemp->DistModePtr = &CCinemaPath::EaseOut;
		break;
	case CCinemaPath::EM_INOUT:
		SetTemp->DistModePtr = &CCinemaPath::EaseInOut;
		break;
	case CCinemaPath::EM_OUTIN:
		SetTemp->DistModePtr = &CCinemaPath::EaseOutIn;
		break;
	default:
		debug_printf("Cinematic mode not found for %d ", data.m_Mode);
		break;
	}

	switch (data.m_Style)
	{
	case CCinemaPath::ES_DEFAULT:
		SetTemp->DistStylePtr = &CCinemaPath::EaseDefault;
		break;
	case CCinemaPath::ES_GROWTH:
		SetTemp->DistStylePtr = &CCinemaPath::EaseGrowth;
		break;
	case CCinemaPath::ES_EXPO:
		SetTemp->DistStylePtr = &CCinemaPath::EaseExpo;
		break;
	case CCinemaPath::ES_CIRCLE:
		SetTemp->DistStylePtr = &CCinemaPath::EaseCircle;
		break;
	case CCinemaPath::ES_SINE:
		SetTemp->DistStylePtr = &CCinemaPath::EaseSine;
		break;
	default:
		debug_printf("Cinematic mode not found for %d !", data.m_Style);
		break;
	}
	UpdateDuration();
}
void CCinemaTrack::AddPath(const CCinemaPath& path)
{
	m_Paths.push_back(path);
	m_Paths.back().BuildSpline();
	m_Paths.back().m_TimeElapsed = 0;

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
			m_CPA->MoveToPointAt(1.f, CalculateRotation());
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
					m_CPA->m_TimeElapsed = Pos;
					break;
				}
			}
		}	//inside limits of path
	}	
	return true;
}
CVector3D CCinemaTrack::CalculateRotation()
{
	if ( m_CPA == m_Paths.begin() )
		return m_StartRotation;
	CVector3D startRotation(0.f, 0.f, 0.f);
	for ( std::vector<CCinemaPath>::iterator it=m_Paths.begin(); it!=m_CPA; ++it )
		startRotation += it->GetData()->m_TotalRotation;
	return startRotation + m_StartRotation;
}

bool CCinemaTrack::Play(float DeltaTime)
{
	m_CPA->m_TimeElapsed += m_Timescale*DeltaTime;
	m_AbsoluteTime += m_Timescale*DeltaTime;

	if (!Validate())
		return false;
	CVector3D rotation = CalculateRotation();
	m_CPA->MoveToPointAt( m_CPA->GetElapsedTime() / m_CPA->GetDuration(), rotation);
	return true;
}

CCinemaManager::CCinemaManager() : m_DrawCurrentSpline(false), 
			m_DrawAllSplines(false), m_Active(true), m_CurrentTrack(false)
{
}

void CCinemaManager::AddTrack(CCinemaTrack track, const CStrW& name)
{
	debug_assert( m_Tracks.find( name ) == m_Tracks.end() );
	m_Tracks[name] = track;
}

void CCinemaManager::QueueTrack(const CStrW& name, bool queue )
{
	if (!m_TrackQueue.empty() && queue == false)
	{
		return;
	}
	else
	{
		debug_assert(HasTrack(name));
		if ( m_Tracks[name].m_Paths.empty() )
			return;
		m_TrackQueue.push_back(m_Tracks[name]);
		m_TrackQueue.back().m_CPA = m_TrackQueue.back().m_Paths.begin();
	}
}
void CCinemaManager::OverrideTrack(const CStrW& name)
{
	m_TrackQueue.clear();
	debug_assert(HasTrack(name));
	if ( m_Tracks[name].m_Paths.empty() )
		return;
	m_TrackQueue.push_back( m_Tracks[name] );
	m_TrackQueue.back().m_CPA = m_TrackQueue.back().m_Paths.begin();
}
void CCinemaManager::SetAllTracks( const std::map<CStrW, CCinemaTrack>& tracks)
{
	m_Tracks = tracks;
	for ( std::map<CStrW, CCinemaTrack>::iterator it=m_Tracks.begin();
			it != m_Tracks.end(); ++it )
	{
		it->second.m_CPA = it->second.m_Paths.begin();
	}
}
void CCinemaManager::SetCurrentTrack(const CStrW& name, bool all, bool current, bool drawLines)
{
	debug_assert(HasTrack(name));
	m_CurrentTrack = &m_Tracks[name];
	m_DrawAllSplines = all;
	m_DrawCurrentSpline = current;
	m_DrawLines = drawLines;
	DrawAllSplines();
}
void CCinemaManager::SetCurrentPath(const int path)
{
	debug_assert(m_CurrentTrack);
	m_CurrentPath = path;
	m_CurrentTrack->m_CPA = m_CurrentTrack->m_Paths.begin() + path;
}
bool CCinemaManager::HasTrack(const CStrW& name) const
{ 
	return m_Tracks.find(name) != m_Tracks.end();
}

void CCinemaManager::DrawAllSplines() const
{
	if ( !(m_DrawAllSplines || m_DrawCurrentSpline) )
		return;
	static const int smoothness = 200;
	
	if ( m_DrawAllSplines )
	{
		for ( std::vector<CCinemaPath>::iterator it = m_CurrentTrack->m_Paths.begin(); 
						it != m_CurrentTrack->m_Paths.end(); ++it )
		{			
			it->DrawSpline(CVector4D(0.f, 0.f, 1.f, 1.f), smoothness, m_DrawLines);
		}
	}
	if ( m_DrawCurrentSpline && m_CurrentTrack->m_CPA != 
									m_CurrentTrack->m_Paths.end() )
	{
		if ( m_DrawAllSplines )
		{
			m_CurrentTrack->m_CPA->DrawSpline(CVector4D(1.f, 0.f, 0.f, 1.f), 
												smoothness, m_DrawLines);
		}
		else
		{
			m_CurrentTrack->m_CPA->DrawSpline(CVector4D(0.f, 0.f, 1.f, 1.f), 
											smoothness, m_DrawLines);
		}
	}
}
void CCinemaManager::MoveToPointAt(float time)
{
	debug_assert(m_CurrentTrack);
	StopPlaying();
	if ( m_CurrentTrack->m_Paths.empty() )
		return;

	m_CurrentTrack->m_CPA->m_TimeElapsed = time;
	if ( !m_CurrentTrack->Validate() )
		return;
	m_CurrentTrack->m_CPA->MoveToPointAt(m_CurrentTrack->m_CPA->m_TimeElapsed / 
		m_CurrentTrack->m_CPA->GetDuration(), m_CurrentTrack->CalculateRotation());
}
void CCinemaManager::MoveToPointAbsolute(float time)
{
	debug_assert(m_CurrentTrack);
	g_Game->GetView()->GetCinema()->StopPlaying();
	if ( m_CurrentTrack->m_Paths.empty() )
		return;

	m_CurrentTrack->m_CPA = m_CurrentTrack->m_Paths.begin();
	m_CurrentTrack->m_AbsoluteTime = m_CurrentTrack->m_CPA->m_TimeElapsed = time;
	
	if (!m_CurrentTrack->ValidateForward())
		return;
	m_CurrentTrack->m_CPA->MoveToPointAt(m_CurrentTrack->m_CPA->m_TimeElapsed / 
	m_CurrentTrack->m_CPA->GetDuration(), m_CurrentTrack->CalculateRotation());
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
