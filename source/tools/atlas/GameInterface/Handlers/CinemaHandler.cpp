#include "precompiled.h"

#include "MessageHandler.h"
#include "../CommandProc.h"
#include "graphics/GameView.h"
#include "ps/Game.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "ps/VFSUtil.h"
#include "lib/res/graphics/tex.h"

#define LOG_CATEGORY "Cinema"

namespace AtlasMessage {

sCinemaTrack ConstructCinemaTrack(const CCinemaTrack& _track)
{
	sCinemaTrack track;
	const CVector3D& rotation = _track.GetRotation();
	track.x = rotation.X;
	track.y = rotation.Y;
	track.z = rotation.Z;
	track.timescale = _track.GetTimeScale();
	track.duration = _track.GetTotalDuration();
	
	return track;
}
sCinemaPath ConstructCinemaPath(const CCinemaData* data)
{
	sCinemaPath path;

	path.x = data->m_TotalRotation.X;
	path.y = data->m_TotalRotation.Y;
	path.z = data->m_TotalRotation.Z;
	path.mode = data->m_Mode;
	path.style = data->m_Style;
	path.growth = data->m_Growth;
	path.change = data->m_Switch;

	return path;
}
CCinemaData ConstructCinemaData(const sCinemaPath& path)
{
	CCinemaData data;
	data.m_TotalRotation = CVector3D(path.x, path.y, path.z);
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
			sCinemaPath path = ConstructCinemaPath(it2->GetData());	//Get data part of path
			const std::vector<SplineData>& nodes = it2->GetAllNodes();

			std::vector<sCinemaSplineNode> atlasNodes;
			
			for ( size_t i=0; i<nodes.size(); ++i )
			{
				atlasNodes.push_back( ConstructCinemaNode(nodes[i]) );
			}
			path.nodes = atlasNodes;
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
		tracks[trackName].SetStartRotation( CVector3D(it->x, it->y, it->z) );
		tracks[trackName].SetTimeScale(it->timescale);
		const std::vector<sCinemaPath> paths = *it->paths;
		
		for ( std::vector<sCinemaPath>::const_iterator it2=paths.begin(); it2!=paths.end(); it2++ )
		{
			const sCinemaPath& atlasPath = *it2;
			const std::vector<sCinemaSplineNode> nodes = *atlasPath.nodes;
			TNSpline spline;
			CCinemaData data = ConstructCinemaData(atlasPath);

			for ( size_t i=0; i<nodes.size(); ++i )
			{	
				spline.AddNode( CVector3D(nodes[i].x, nodes[i].y, nodes[i].z), nodes[i].t );
			}
			tracks[trackName].AddPath(data, spline);
		}
	}
	g_Game->GetView()->GetCinema()->SetAllTracks(tracks);
}

QUERYHANDLER(GetCinemaIcons)
{
	VFSUtil::FileList files;
	VFSUtil::FindFiles("art/textures/ui/session/icons/cinematic/","", files);
	FileIOBuf buf;
	size_t bufsize;
	std::vector<sCinemaIcon> iconList;

	for ( VFSUtil::FileList::iterator it=files.begin(); it != files.end(); it++ )
	{
		if ( tex_is_known_extension(*it) )
		{
			const char* file = it->c_str();

			if ( vfs_load(file, buf, bufsize) < 0 )
			{	
				LOG( ERROR, LOG_CATEGORY, "Failure on loading cinematic icon %s", file );
				file_buf_free(buf);
				continue;
			}
			sCinemaIcon icon;
			std::wstring name( CStrW( *it->AfterLast("/").BeforeFirst(".").c_str() ) );
			std::vector<unsigned char> data;
			data.resize(sizeof(data));

			//Copy the buffer to the icon
			for ( size_t i=0; *buf++; i++ )
			{
				data.push_back(*buf);
			}
			file_buf_free(buf);
			icon.name = name;
			icon.imageData = data;
			iconList.push_back(icon);
		}
	}
	msg->images = iconList;
}

MESSAGEHANDLER(CinemaMovement)
{
	CCinemaManager* manager = g_Game->GetView()->GetCinema();
	CCinemaTrack* track = manager->GetTrack(*msg->track);

	if ( msg->mode == eCinemaMovementMode::SMOOTH )
		manager->OverrideTrack(*msg->track);	
	else if ( msg->mode == eCinemaMovementMode::IMMEDIATE_PATH )
		track->MoveToPointAt(msg->t);
	else if ( msg->mode == eCinemaMovementMode::IMMEDIATE_TRACK )
		track->MoveToPointAbsolute(msg->t);
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
