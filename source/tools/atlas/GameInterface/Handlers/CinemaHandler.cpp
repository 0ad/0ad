/* Copyright (C) 2017 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "MessageHandler.h"
#include "../CommandProc.h"
#include "../GameLoop.h"
#include "../View.h"
#include "graphics/Camera.h"
#include "graphics/CinemaManager.h"
#include "graphics/GameView.h"
#include "ps/Game.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "maths/Vector2D.h"
#include "maths/Vector3D.h"
#include "lib/res/graphics/ogl_tex.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpCinemaManager.h"


namespace AtlasMessage {

const float MINIMAL_SCREEN_DISTANCE = 5.f;

sCinemaPath ConstructCinemaPath(const CCinemaPath* source)
{
	sCinemaPath path;
	const CCinemaData* data = source->GetData();
	//path.mode = data->m_Mode;
	//path.style = data->m_Style;
	path.growth = data->m_Growth;
	path.timescale = data->m_Timescale.ToFloat();
	path.change = data->m_Switch;

	return path;
}
CCinemaData ConstructCinemaData(const sCinemaPath& path)
{
	CCinemaData data;
	data.m_Growth = data.m_GrowthCount = path.growth;
	data.m_Switch = path.change;
	//data.m_Mode = path.mode;
	//data.m_Style = path.style;

	return data;
}
sCinemaSplineNode ConstructCinemaNode(const SplineData& data)
{
	sCinemaSplineNode node;
	node.px = data.Position.X.ToFloat();
	node.py = data.Position.Y.ToFloat();
	node.pz = data.Position.Z.ToFloat();

	node.rx = data.Rotation.X.ToFloat();
	node.ry = data.Rotation.Y.ToFloat();
	node.rz = data.Rotation.Z.ToFloat();
	node.t = data.Distance.ToFloat();

	return node;
}

std::vector<sCinemaPath> GetCurrentPaths()
{
	std::vector<sCinemaPath> atlasPaths;
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (!cmpCinemaManager)
		return atlasPaths;
	const std::map<CStrW, CCinemaPath>& paths = cmpCinemaManager->GetPaths();

	for ( std::map<CStrW, CCinemaPath>::const_iterator it=paths.begin(); it!=paths.end(); ++it  )
	{
		sCinemaPath path = ConstructCinemaPath(&it->second);
		path.name = it->first;

		const std::vector<SplineData>& nodes = it->second.GetAllNodes();
		std::vector<sCinemaSplineNode> atlasNodes;

		for ( size_t i=0; i<nodes.size(); ++i )
			atlasNodes.push_back( ConstructCinemaNode(nodes[i]) );

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
		atlasPaths.push_back(path);
	}
	return atlasPaths;
}

void SetCurrentPaths(const std::vector<sCinemaPath>& atlasPaths)
{
	std::map<CStrW, CCinemaPath> paths;

	for ( std::vector<sCinemaPath>::const_iterator it=atlasPaths.begin(); it!=atlasPaths.end(); ++it )
	{
		CStrW pathName(*it->name);
		paths[pathName] = CCinemaPath();
		paths[pathName].SetTimescale(fixed::FromFloat(it->timescale));

		const sCinemaPath& atlasPath = *it;
		const std::vector<sCinemaSplineNode> nodes = *atlasPath.nodes;
		TNSpline spline;
		CCinemaData data = ConstructCinemaData(atlasPath);

 		for ( size_t j=0; j<nodes.size(); ++j )
		{
			spline.AddNode(CFixedVector3D(fixed::FromFloat(nodes[j].px), fixed::FromFloat(nodes[j].py), fixed::FromFloat(nodes[j].pz)),
				CFixedVector3D(fixed::FromFloat(nodes[j].rx), fixed::FromFloat(nodes[j].ry), fixed::FromFloat(nodes[j].rz)), fixed::FromFloat(nodes[j].t));
		}
		paths[pathName] = CCinemaPath(data, spline, TNSpline());
	}

	CmpPtr<ICmpCinemaManager> cmpCinemaManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (cmpCinemaManager)
		cmpCinemaManager->SetPaths(paths);
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
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (!cmpCinemaManager)
		return;

	if (msg->mode == eCinemaEventMode::SMOOTH)
	{
		cmpCinemaManager->AddCinemaPathToQueue(*msg->path);
	}
	else if ( msg->mode == eCinemaEventMode::RESET )
	{
//		g_Game->GetView()->ResetCamera();
	}
	else
		ENSURE(false);
}

BEGIN_COMMAND(AddCinemaPath)
{
	void Do()
	{
		CmpPtr<ICmpCinemaManager> cmpCinemaManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (!cmpCinemaManager)
			return;

		CCinemaData pathData;
		pathData.m_Name = *msg->pathName;
		pathData.m_Timescale = fixed::FromInt(1);
		pathData.m_Orientation = L"target";
		pathData.m_Mode = L"ease_inout";
		pathData.m_Style = L"default";

		CVector3D focus = g_Game->GetView()->GetCamera()->GetFocus();
		CFixedVector3D target(
			fixed::FromFloat(focus.X),
			fixed::FromFloat(focus.Y),
			fixed::FromFloat(focus.Z)
		);

		CVector3D camera = g_Game->GetView()->GetCamera()->GetOrientation().GetTranslation();
		CFixedVector3D position(
			fixed::FromFloat(camera.X),
			fixed::FromFloat(camera.Y),
			fixed::FromFloat(camera.Z)
		);

		TNSpline positionSpline;
		positionSpline.AddNode(position, CFixedVector3D(), fixed::FromInt(0));

		TNSpline targetSpline;
		targetSpline.AddNode(target, CFixedVector3D(), fixed::FromInt(0));

		cmpCinemaManager->AddPath(CCinemaPath(pathData, positionSpline, targetSpline));
	}
	void Redo()
	{
	}
	void Undo()
	{
	}
};
END_COMMAND(AddCinemaPath)

BEGIN_COMMAND(DeleteCinemaPath)
{
	void Do()
	{
		CmpPtr<ICmpCinemaManager> cmpCinemaManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (!cmpCinemaManager)
			return;

		cmpCinemaManager->DeletePath(*msg->pathName);
	}
	void Redo()
	{
	}
	void Undo()
	{
	}
};
END_COMMAND(DeleteCinemaPath)

BEGIN_COMMAND(SetCinemaPaths)
{
	std::vector<sCinemaPath> m_oldPaths, m_newPaths;
	void Do()
	{
		m_oldPaths = GetCurrentPaths();
		m_newPaths = *msg->paths;
		Redo();
	}
	void Redo()
	{
		SetCurrentPaths(m_newPaths);
	}
	void Undo()
	{
		SetCurrentPaths(m_oldPaths);
	}
};
END_COMMAND(SetCinemaPaths)

BEGIN_COMMAND(SetCinemaPathsDrawing)
{
	void Do()
	{
		if (g_Game && g_Game->GetView() && g_Game->GetView()->GetCinema())
			g_Game->GetView()->GetCinema()->SetPathsDrawing(msg->drawPaths);
	}

	void Redo()
	{
	}

	void Undo()
	{
	}
};
END_COMMAND(SetCinemaPathsDrawing)

static CVector3D GetNearestPointToScreenCoords(const CVector3D& base, const CVector3D& dir, const CVector2D& screen, float lower = -1e5, float upper = 1e5)
{
	// It uses a ternary search, because an intersection of cylinders is the complex task
	for (int i = 0; i < 64; ++i)
	{
		float delta = (upper - lower) / 3.0;
		float middle1 = lower + delta, middle2 = lower + 2.0f * delta;
		CVector3D p1 = base + dir * middle1, p2 = base + dir * middle2;
		CVector2D s1, s2;
		g_Game->GetView()->GetCamera()->GetScreenCoordinates(p1, s1.X, s1.Y);
		g_Game->GetView()->GetCamera()->GetScreenCoordinates(p2, s2.X, s2.Y);
		if ((s1 - screen).Length() < (s2 - screen).Length())
			upper = middle2;
		else
			lower = middle1;
	}
	return base + dir * upper;
}

#define GET_PATH_NODE_WITH_VALIDATION() \
	int index = msg->node->index; \
	if (index < 0) \
		return; \
	CStrW name = *msg->node->name; \
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY); \
	if (!cmpCinemaManager || !cmpCinemaManager->HasPath(name)) \
		return; \
	const CCinemaPath& path = cmpCinemaManager->GetPaths().find(name)->second; \
	if (!msg->node->targetNode) \
	{ \
		if (index >= (int)path.GetAllNodes().size()) \
			return; \
	} \
	else \
	{ \
		if (index >= (int)path.GetTargetSpline().GetAllNodes().size()) \
			return; \
	}

BEGIN_COMMAND(AddPathNode)
{
	void Do()
	{
		GET_PATH_NODE_WITH_VALIDATION();

		CCinemaData data = *path.GetData();
		TNSpline positionSpline = path;
		TNSpline targetSpline = path.GetTargetSpline();
		TNSpline& spline = msg->node->targetNode ? targetSpline : positionSpline;

		CVector3D focus = g_Game->GetView()->GetCamera()->GetFocus();
		CFixedVector3D target(
			fixed::FromFloat(focus.X),
			fixed::FromFloat(focus.Y),
			fixed::FromFloat(focus.Z)
		);
		spline.InsertNode(index, target, CFixedVector3D(), fixed::FromInt(1));
		
		spline.BuildSpline();
		cmpCinemaManager->DeletePath(name);
		cmpCinemaManager->AddPath(CCinemaPath(data, positionSpline, targetSpline));
	}

	void Redo()
	{
	}

	void Undo()
	{
	}
};
END_COMMAND(AddPathNode)

BEGIN_COMMAND(DeletePathNode)
{
	void Do()
	{
		GET_PATH_NODE_WITH_VALIDATION();

		CCinemaData data = *path.GetData();
		TNSpline positionSpline = path;
		TNSpline targetSpline = path.GetTargetSpline();
		TNSpline& spline = msg->node->targetNode ? targetSpline : positionSpline;
		if (spline.GetAllNodes().size() <= 1)
			return;

		spline.RemoveNode(index);
		spline.BuildSpline();
		cmpCinemaManager->DeletePath(name);
		cmpCinemaManager->AddPath(CCinemaPath(data, positionSpline, targetSpline));

		g_AtlasGameLoop->view->SetParam(L"movetool", false);
	}

	void Redo()
	{
	}

	void Undo()
	{
	}
};
END_COMMAND(DeletePathNode)

BEGIN_COMMAND(MovePathNode)
{
	void Do()
	{
		int axis = msg->axis;
		if (axis == AXIS_INVALID)
			return;

		GET_PATH_NODE_WITH_VALIDATION();

		CCinemaData data = *path.GetData();
		TNSpline positionSpline = path;
		TNSpline targetSpline = path.GetTargetSpline();
		TNSpline& spline = msg->node->targetNode ? targetSpline : positionSpline;

		// Get shift of the tool by the cursor movement
		CFixedVector3D pos = spline.GetAllNodes()[index].Position;
		CVector3D position(
			pos.X.ToFloat(),
			pos.Y.ToFloat(),
			pos.Z.ToFloat()
		);
		CVector3D axisDirection(axis & AXIS_X, axis & AXIS_Y, axis & AXIS_Z);
		CVector2D from, to;
		msg->from->GetScreenSpace(from.X, from.Y);
		msg->to->GetScreenSpace(to.X, to.Y);
		CVector3D shift(
			GetNearestPointToScreenCoords(position, axisDirection, to) -
			GetNearestPointToScreenCoords(position, axisDirection, from)
		);

		// Change, rebuild and update the path
		position += shift;
		pos += CFixedVector3D(
			fixed::FromFloat(shift.X),
			fixed::FromFloat(shift.Y),
			fixed::FromFloat(shift.Z)
		);
		spline.UpdateNodePos(index, pos);
		spline.BuildSpline();
		cmpCinemaManager->DeletePath(name);
		cmpCinemaManager->AddPath(CCinemaPath(data, positionSpline, targetSpline));

		// Update visual tool coordinates
		g_AtlasGameLoop->view->SetParam(L"movetool_x", position.X);
		g_AtlasGameLoop->view->SetParam(L"movetool_y", position.Y);
		g_AtlasGameLoop->view->SetParam(L"movetool_z", position.Z);
	}

	void Redo()
	{
	}

	void Undo()
	{
	}
};
END_COMMAND(MovePathNode)

QUERYHANDLER(GetCinemaPaths)
{
	msg->paths = GetCurrentPaths();
}

static bool isPathNodePicked(const TNSpline& spline, const CVector2D& cursor, AtlasMessage::sCinemaPathNode& node, bool targetNode)
{
	for (size_t i = 0; i < spline.GetAllNodes().size(); ++i)
	{
		const SplineData& data = spline.GetAllNodes()[i];
		CVector3D position(
			data.Position.X.ToFloat(),
			data.Position.Y.ToFloat(),
			data.Position.Z.ToFloat()
		);
		CVector2D screen_pos;
		g_Game->GetView()->GetCamera()->GetScreenCoordinates(position, screen_pos.X, screen_pos.Y);
		if ((screen_pos - cursor).Length() < MINIMAL_SCREEN_DISTANCE)
		{
			node.index = i;
			node.targetNode = targetNode;
			g_AtlasGameLoop->view->SetParam(L"movetool", true);
			g_AtlasGameLoop->view->SetParam(L"movetool_x", position.X);
			g_AtlasGameLoop->view->SetParam(L"movetool_y", position.Y);
			g_AtlasGameLoop->view->SetParam(L"movetool_z", position.Z);
			return true;
		}
	}
	return false;
}

QUERYHANDLER(PickPathNode)
{
	AtlasMessage::sCinemaPathNode node;
	CmpPtr<ICmpCinemaManager> cmpCinemaManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (!cmpCinemaManager)
	{
		msg->node = node;
		return;
	}

	CVector2D cursor;
	msg->pos->GetScreenSpace(cursor.X, cursor.Y);

	for (const std::pair<CStrW, CCinemaPath>& p : cmpCinemaManager->GetPaths())
	{
		const CCinemaPath& path = p.second;		
		if (isPathNodePicked(path, cursor, node, false) || isPathNodePicked(path.GetTargetSpline(), cursor, node, true))
		{
			node.name = path.GetName();
			msg->node = node;
			return;
		}
	}
	msg->node = node;
	g_AtlasGameLoop->view->SetParam(L"movetool", false);
}

static bool isAxisPicked(const CVector3D& base, const CVector3D& direction, float length, const CVector2D& cursor)
{
	CVector3D position = GetNearestPointToScreenCoords(base, direction, cursor, 0, length);
	CVector2D screen_position;
	g_Game->GetView()->GetCamera()->GetScreenCoordinates(position, screen_position.X, screen_position.Y);
	return (cursor - screen_position).Length() < MINIMAL_SCREEN_DISTANCE;
}

QUERYHANDLER(PickAxis)
{
	msg->axis = AXIS_INVALID;

	GET_PATH_NODE_WITH_VALIDATION();

	const TNSpline& spline = msg->node->targetNode ? path.GetTargetSpline() : path;
	CFixedVector3D pos = spline.GetAllNodes()[index].Position;
	CVector3D position(pos.X.ToFloat(), pos.Y.ToFloat(), pos.Z.ToFloat());
	CVector3D camera = g_Game->GetView()->GetCamera()->GetOrientation().GetTranslation();
	float scale = (position - camera).Length() / 10.0;

	CVector2D cursor;
	msg->pos->GetScreenSpace(cursor.X, cursor.Y);
	if (isAxisPicked(position, CVector3D(1, 0, 0), scale, cursor))
		msg->axis = AXIS_X;
	else if (isAxisPicked(position, CVector3D(0, 1, 0), scale, cursor))
		msg->axis = AXIS_Y;
	else if (isAxisPicked(position, CVector3D(0, 0, 1), scale, cursor))
		msg->axis = AXIS_Z;
}

MESSAGEHANDLER(ClearPathNodePreview)
{
	UNUSED2(msg);
	g_AtlasGameLoop->view->SetParam(L"movetool", false);
}

}
