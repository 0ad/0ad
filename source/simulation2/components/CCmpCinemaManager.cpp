/* Copyright (C) 2016 Wildfire Games.
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

#include "simulation2/system/Component.h"
#include "ICmpCinemaManager.h"

#include "graphics/GameView.h"
#include "graphics/CinemaManager.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/Simulation2.h"


class CCmpCinemaManager : public ICmpCinemaManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update);
	}

	DEFAULT_COMPONENT_ALLOCATOR(CinemaManager)

	static std::string GetSchema()
	{
		return "<a:component type='system'/>"
			"<empty/>"
			;
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		// ...
	}

	virtual void Deinit()
	{
		// ...
	}

	virtual void Serialize(ISerializer& serialize)
	{
		if (!g_Game || !g_Game->GetView())
			return;

		CinematicSimulationData* p_CinematicSimulationData = g_Game->GetView()->GetCinema()->GetCinematicSimulationData();
		serialize.Bool("MapRevealed", p_CinematicSimulationData->m_MapRevealed);
		serialize.NumberU32_Unbounded("NumberOfPaths", p_CinematicSimulationData->m_Paths.size());
		for (auto it : p_CinematicSimulationData->m_Paths)
		{
			CCinemaPath& path = it.second;
			const CCinemaData* data = path.GetData();

			// TODO: maybe implement String_Unbounded
			serialize.String("PathName", data->m_Name, 1, 2048);
			serialize.String("PathOrientation", data->m_Orientation, 1, 2048);
			serialize.String("PathMode", data->m_Mode, 1, 2048);
			serialize.String("PathStyle", data->m_Style, 1, 2048);
			serialize.NumberFixed_Unbounded("PathTimescale", data->m_Timescale);
			serialize.Bool("LookAtTarget", data->m_LookAtTarget);

			serialize.NumberU32("NumberOfNodes", path.GetAllNodes().size(), 1, MAX_SPLINE_NODES);
			const std::vector<SplineData>& nodes = path.GetAllNodes();
			for (size_t i = 0; i < nodes.size(); ++i)
			{
				if (i > 0)
					serialize.NumberFixed_Unbounded("NodeDeltaTime", nodes[i - 1].Distance);
				else
					serialize.NumberFixed_Unbounded("NodeDeltaTime", fixed::Zero());
				serialize.NumberFixed_Unbounded("PositionX", nodes[i].Position.X);
				serialize.NumberFixed_Unbounded("PositionY", nodes[i].Position.Y);
				serialize.NumberFixed_Unbounded("PositionZ", nodes[i].Position.Z);

				serialize.NumberFixed_Unbounded("RotationX", nodes[i].Rotation.X);
				serialize.NumberFixed_Unbounded("RotationY", nodes[i].Rotation.Y);
				serialize.NumberFixed_Unbounded("RotationZ", nodes[i].Rotation.Z);
			}

			if (!data->m_LookAtTarget)
				continue;

			const std::vector<SplineData>& targetNodes = path.GetTargetSpline().GetAllNodes();
			serialize.NumberU32("NumberOfTargetNodes", targetNodes.size(), 1, MAX_SPLINE_NODES);
			for (size_t i = 0; i < targetNodes.size(); ++i)
			{
				if (i > 0)
					serialize.NumberFixed_Unbounded("NodeDeltaTime", targetNodes[i - 1].Distance);
				else
					serialize.NumberFixed_Unbounded("NodeDeltaTime", fixed::Zero());
				serialize.NumberFixed_Unbounded("PositionX", targetNodes[i].Position.X);
				serialize.NumberFixed_Unbounded("PositionY", targetNodes[i].Position.Y);
				serialize.NumberFixed_Unbounded("PositionZ", targetNodes[i].Position.Z);
			}
		}
	}

	virtual void Deserialize(const CParamNode& UNUSED(paramNode), IDeserializer& deserialize)
	{
		if (!g_Game || !g_Game->GetView())
			return;

		CinematicSimulationData* p_CinematicSimulationData = g_Game->GetView()->GetCinema()->GetCinematicSimulationData();
		deserialize.Bool("MapRevealed", p_CinematicSimulationData->m_MapRevealed);
		uint32_t numberOfPaths = 0;
		deserialize.NumberU32_Unbounded("NumberOfPaths", numberOfPaths);
		for (uint32_t i = 0; i < numberOfPaths; ++i)
		{
			CCinemaData data;

			deserialize.String("PathName", data.m_Name, 1, 2048);
			deserialize.String("PathOrientation", data.m_Orientation, 1, 2048);
			deserialize.String("PathMode", data.m_Mode, 1, 2048);
			deserialize.String("PathStyle", data.m_Style, 1, 2048);
			deserialize.NumberFixed_Unbounded("PathTimescale", data.m_Timescale);
			deserialize.Bool("LookAtTarget", data.m_LookAtTarget);

			TNSpline pathSpline, targetSpline;
			uint32_t numberOfNodes = 0;
			deserialize.NumberU32("NumberOfNodes", numberOfNodes, 1, MAX_SPLINE_NODES);
			for (uint32_t j = 0; j < numberOfNodes; ++j)
			{
				SplineData node;
				deserialize.NumberFixed_Unbounded("NodeDeltaTime", node.Distance);

				deserialize.NumberFixed_Unbounded("PositionX", node.Position.X);
				deserialize.NumberFixed_Unbounded("PositionY", node.Position.Y);
				deserialize.NumberFixed_Unbounded("PositionZ", node.Position.Z);

				deserialize.NumberFixed_Unbounded("RotationX", node.Rotation.X);
				deserialize.NumberFixed_Unbounded("RotationY", node.Rotation.Y);
				deserialize.NumberFixed_Unbounded("RotationZ", node.Rotation.Z);

				pathSpline.AddNode(node.Position, node.Rotation, node.Distance);
			}

			if (data.m_LookAtTarget)
			{
				uint32_t numberOfTargetNodes = 0;
				deserialize.NumberU32("NumberOfTargetNodes", numberOfTargetNodes, 1, MAX_SPLINE_NODES);
				for (uint32_t j = 0; j < numberOfTargetNodes; ++j)
				{
					SplineData node;
					deserialize.NumberFixed_Unbounded("NodeDeltaTime", node.Distance);

					deserialize.NumberFixed_Unbounded("PositionX", node.Position.X);
					deserialize.NumberFixed_Unbounded("PositionY", node.Position.Y);
					deserialize.NumberFixed_Unbounded("PositionZ", node.Position.Z);

					targetSpline.AddNode(node.Position, CFixedVector3D(), node.Distance);
				}
			}

			// Construct cinema path with data gathered
			CCinemaPath path(data, pathSpline, targetSpline);
			p_CinematicSimulationData->m_Paths[data.m_Name] = path;
		}
		g_Game->GetView()->GetCinema()->SetEnabled(p_CinematicSimulationData->m_Enabled);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		if (!g_Game || !g_Game->GetView())
			return;

		switch (msg.GetType())
		{
		case MT_Update:
		{
			const CMessageUpdate &msgData = static_cast<const CMessageUpdate&>(msg);
			CinematicSimulationData* pCinematicSimulationData = g_Game->GetView()->GetCinema()->GetCinematicSimulationData();
			if (!pCinematicSimulationData->m_Enabled)
				break;

			pCinematicSimulationData->m_ElapsedTime += msgData.turnLength;
			pCinematicSimulationData->m_CurrentPathElapsedTime += msgData.turnLength;
			if (pCinematicSimulationData->m_CurrentPathElapsedTime >= pCinematicSimulationData->m_PathQueue.front().GetDuration())
			{
				CMessageCinemaPathEnded msgCinemaPathEnded(pCinematicSimulationData->m_PathQueue.front().GetName());
				pCinematicSimulationData->m_PathQueue.pop_front();
				g_Game->GetSimulation2()->PostMessage(SYSTEM_ENTITY, msgCinemaPathEnded);
				pCinematicSimulationData->m_CurrentPathElapsedTime = fixed::Zero();
				if (!pCinematicSimulationData->m_PathQueue.empty())
					pCinematicSimulationData->m_PathQueue.front().Reset();
			}
			if (pCinematicSimulationData->m_ElapsedTime >= pCinematicSimulationData->m_TotalTime)
			{
				pCinematicSimulationData->m_CurrentPathElapsedTime = fixed::Zero();
				pCinematicSimulationData->m_ElapsedTime = fixed::Zero();
				pCinematicSimulationData->m_TotalTime = fixed::Zero();
				g_Game->GetView()->GetCinema()->SetEnabled(false);
				g_Game->GetSimulation2()->PostMessage(SYSTEM_ENTITY, CMessageCinemaQueueEnded());
			}
			break;
		}
		default:
			break;
		}
	}

	virtual void AddCinemaPathToQueue(const CStrW& name)
	{
		if (!g_Game || !g_Game->GetView())
			return;
		g_Game->GetView()->GetCinema()->AddPathToQueue(name);
		CinematicSimulationData* pCinematicSimulationData = g_Game->GetView()->GetCinema()->GetCinematicSimulationData();
		if (pCinematicSimulationData->m_PathQueue.size() == 1)
			pCinematicSimulationData->m_PathQueue.front().Reset();
		pCinematicSimulationData->m_TotalTime += pCinematicSimulationData->m_Paths[name].GetDuration();
	}

	virtual void Play()
	{
		if (!g_Game || !g_Game->GetView())
			return;
		g_Game->GetView()->GetCinema()->Play();
		g_Game->GetView()->GetCinema()->SetEnabled(true);
	}

	virtual void Stop()
	{
		if (!g_Game || !g_Game->GetView())
			return;
		g_Game->GetView()->GetCinema()->Stop();
		g_Game->GetView()->GetCinema()->SetEnabled(false);
	}

};

REGISTER_COMPONENT_TYPE(CinemaManager)
