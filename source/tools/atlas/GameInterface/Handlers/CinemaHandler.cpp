#include "precompiled.h"

#include "MessageHandler.h"
#include "../CommandProc.h"
#include "graphics/Camera.h"
#include "graphics/CinemaTrack.h"
#include "graphics/GameView.h"
#include "ps/Game.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "ps/VFSUtil.h"
#include "maths/Quaternion.h"
#include "lib/res/graphics/ogl_tex.h"

#define LOG_CATEGORY "Cinema"

namespace AtlasMessage {

sCinemaTrack ConstructCinemaTrack(const CCinemaTrack& data)
{
	sCinemaTrack track;
	const CVector3D& rotation = data.GetRotation();
	track.x = RADTODEG(rotation.X);
	track.y = RADTODEG(rotation.Y);
	track.z = RADTODEG(rotation.Z);
	track.timescale = data.GetTimescale();
	track.duration = data.GetTotalDuration();
	
	return track;
}
sCinemaPath ConstructCinemaPath(const CCinemaPath* source)
{
	sCinemaPath path;
	const CCinemaData* data = source->GetData();

	path.x = RADTODEG(data->m_TotalRotation.X);
	path.y = RADTODEG(data->m_TotalRotation.Y);
	path.z = RADTODEG(data->m_TotalRotation.Z);
	path.mode = data->m_Mode;
	path.style = data->m_Style;
	path.growth = data->m_Growth;
	path.change = data->m_Switch;

	return path;
}
CCinemaData ConstructCinemaData(const sCinemaPath& path)
{
	CCinemaData data;
	data.m_TotalRotation = CVector3D(DEGTORAD(path.x), DEGTORAD(path.y), 
													DEGTORAD(path.z));
	data.m_Growth = data.m_GrowthCount = path.growth;
	data.m_Switch = path.change;
	data.m_Mode = path.mode;
	data.m_Style = path.style;
	
	return data;
}
sCinemaSplineNode ConstructCinemaNode(const SplineData& data)
{
	sCinemaSplineNode node;
	node.x = data.Position.X;
	node.y = data.Position.Y;
	node.z = data.Position.Z;
	node.t = data.Distance;
	
	return node;
}

std::vector<sCinemaTrack> GetCurrentTracks()
{
	const std::map<CStrW, CCinemaTrack>& tracks = g_Game->GetView()->GetCinema()->GetAllTracks();
	std::vector<sCinemaTrack> atlasTracks;

	for ( std::map<CStrW, CCinemaTrack>::const_iterator it=tracks.begin(); it!=tracks.end(); it++  )
	{
		sCinemaTrack atlasTrack = ConstructCinemaTrack(it->second);

		atlasTrack.name = it->first;
		const std::vector<CCinemaPath>& paths = it->second.GetAllPaths();

		std::vector<sCinemaPath> atlasPaths;

		for ( std::vector<CCinemaPath>::const_iterator it2=paths.begin(); it2!=paths.end(); it2++ )
		{
			sCinemaPath path = ConstructCinemaPath(&*it2);	
			const std::vector<SplineData>& nodes = it2->GetAllNodes();

			std::vector<sCinemaSplineNode> atlasNodes;
			
			for ( size_t i=0; i<nodes.size(); ++i )
			{
				atlasNodes.push_back( ConstructCinemaNode(nodes[i]) );
			}
			if ( !atlasNodes.empty() )
			{
				float back = atlasNodes.back().t;
				if ( atlasNodes.size() > 2 )
				{
					for ( size_t i=atlasNodes.size()-2; i>0; --i )
						atlasNodes[i].t = atlasNodes[i-1].t;
				}
				atlasNodes.back().t = atlasNodes.front().t;
				atlasNodes.front().t = back;
			}
			path.nodes = atlasNodes;
			path.duration = it2->GetDuration();
			atlasPaths.push_back(path);
		}
		atlasTrack.paths = atlasPaths;
		atlasTracks.push_back(atlasTrack);
	}
	return atlasTracks;
}
void SetCurrentTracks(const std::vector<sCinemaTrack>& atlasTracks)
{
	std::map<CStrW, CCinemaTrack> tracks;
	
	for ( std::vector<sCinemaTrack>::const_iterator it=atlasTracks.begin(); it!=atlasTracks.end(); it++ )
	{
		CStrW trackName(*it->name);
		tracks[trackName] = CCinemaTrack();
		tracks[trackName].SetStartRotation( CVector3D(DEGTORAD(it->x),
									DEGTORAD(it->y), DEGTORAD(it->z)) );
		tracks[trackName].SetTimescale(it->timescale);
		const std::vector<sCinemaPath> paths = *it->paths;
		size_t i=0;

		for ( std::vector<sCinemaPath>::const_iterator it2=paths.begin();
				it2!=paths.end(); it2++, ++i )
		{
			const sCinemaPath& atlasPath = *it2;
			const std::vector<sCinemaSplineNode> nodes = *atlasPath.nodes;
			TNSpline spline;
			CCinemaData data = ConstructCinemaData(atlasPath);

			for ( size_t j=0; j<nodes.size(); ++j )
			{	
				spline.AddNode( CVector3D(nodes[j].x, nodes[j].y, nodes[j].z), nodes[j].t );
			}
			tracks[trackName].AddPath(data, spline);
		}
	}
	g_Game->GetView()->GetCinema()->SetAllTracks(tracks);
}
QUERYHANDLER(GetCameraInfo)
{
	sCameraInfo info;
	CMatrix3D* cam = &g_Game->GetView()->GetCamera()->m_Orientation;

	CQuaternion quatRot = cam->GetRotation();
	quatRot.Normalize();
	CVector3D rotation = quatRot.ToEulerAngles();
	rotation.X = RADTODEG(rotation.X); 
	rotation.Y = RADTODEG(rotation.Y);
	rotation.Z = RADTODEG(rotation.Z);
	CVector3D translation = cam->GetTranslation();
	
	info.pX = translation.X;
	info.pY = translation.Y;
	info.pZ = translation.Z;
	info.rX = rotation.X;
	info.rY = rotation.Y;
	info.rZ = rotation.Z;
	msg->info = info;
}

MESSAGEHANDLER(CinemaEvent)
{
	CCinemaManager* manager = g_Game->GetView()->GetCinema();
	manager->SetCurrentTrack(*msg->track, msg->drawAll, 
				msg->drawCurrent, msg->lines);

	if ( msg->mode == eCinemaEventMode::SMOOTH )
		manager->OverrideTrack(*msg->track);	
	else if ( msg->mode == eCinemaEventMode::IMMEDIATE_PATH )
		manager->MoveToPointAt(msg->t);
	else if ( msg->mode == eCinemaEventMode::IMMEDIATE_TRACK )
		manager->MoveToPointAbsolute(msg->t);
	else
		manager->SetCurrentPath((int)msg->t);
}
			
BEGIN_COMMAND(SetCinemaTracks)
{
	std::vector<sCinemaTrack> m_oldTracks, m_newTracks;
	void Do()
	{
		m_oldTracks = GetCurrentTracks();
		m_newTracks = *msg->tracks;
		Redo();
	}
	void Redo()
	{
		SetCurrentTracks(m_newTracks);
	}
	void Undo()
	{
		SetCurrentTracks(m_oldTracks);
	}
};
END_COMMAND(SetCinemaTracks)

QUERYHANDLER(GetCinemaTracks)
{
	msg->tracks = GetCurrentTracks();
}

}
