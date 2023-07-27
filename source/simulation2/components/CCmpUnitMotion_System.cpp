/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "CCmpUnitMotion.h"
#include "CCmpUnitMotionManager.h"

#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <vector>

#define DEBUG_STATS 0
#define DEBUG_RENDER 0
#define DEBUG_RENDER_ALL_PUSH 0

// NB: this TU contains the CCmpUnitMotion/CCmpUnitMotionManager couple.
// In practice, UnitMotionManager functions need access to the full implementation of UnitMotion,
// but UnitMotion needs access to MotionState (defined in UnitMotionManager).
// To avoid inclusion issues, implementation of UnitMotionManager that uses UnitMotion is here.

namespace {
/**
 * Units push within their square and neighboring squares (except diagonals). This is the size of each square (in meters).
 * I have tested grid sizes from 10 up to 80 and overall it made little difference to the performance,
 * mostly, I suspect, because pushing is generally dwarfed by regular motion costs.
 * However, the algorithm remains n^2 in comparisons so it's probably best to err on the side of smaller grids, which will have lower spikes.
 * The balancing act is between comparisons, unordered_set insertions and unordered_set iterations.
 * For these reasons, a value of 20 which is rather small but not overly so was chosen.
 */
constexpr int PUSHING_GRID_SIZE = 20;

/**
 * For pushing, treat the clearances as a circle - they're defined as squares,
 * so we'll take the circumscribing square (approximately).
 * Clerances are also full-width instead of half, so we want to divide by two. sqrt(2)/2 is about 0.71 < 5/7.
 */
constexpr entity_pos_t PUSHING_CORRECTION = entity_pos_t::FromFraction(5, 7);

/**
 * Arbitrary constant used to reduce pushing to levels that won't break physics for our turn length.
 */
constexpr int PUSHING_REDUCTION_FACTOR = 2;

/**
 * Maximum distance-related multiplier.
 * NB: this value interacts with the "minimal pushing" force,
 * as two perfectly overlapping units exert MAX_DISTANCE_FACTOR * Turn length in ms / REDUCTION_FACTOR
 * of force on each other each turn. If this is below the minimal pushing force, any 2 units can entirely overlap.
 */
constexpr entity_pos_t MAX_DISTANCE_FACTOR = entity_pos_t::FromFraction(5, 2);

/**
 * Maximum pushing multiplier for a single push calculation.
 * This exists for numerical stability of the system between a lightweight and a heavy unit.
 */
constexpr int MAX_PUSHING_MULTIPLIER = 4;

/**
 * When two units collide, if their movement dot product is below this value, give them a perpendicular nudge instead of trying to push in the regular way.
 */
constexpr entity_pos_t PERPENDICULAR_NUDGE_THRESHOLD = entity_pos_t::FromFraction(-1, 10);

/**
 * Pushing is dampened by pushing pressure, but this is capped so that units still get pushed.
 */
constexpr int MAX_PUSH_DAMPING_PRESSURE = 160;
static_assert(MAX_PUSH_DAMPING_PRESSURE < CCmpUnitMotionManager::MAX_PRESSURE);

/**
 * When units are obstructed because they're being pushed away from where they want to go,
 * raise the pushing pressure to at least this value.
 */
constexpr int MIN_PRESSURE_IF_OBSTRUCTED = 80;

/**
 * These two numbers are used to calculate pushing pressure between two units.
 */
constexpr entity_pos_t PRESSURE_STATIC_FACTOR =  entity_pos_t::FromInt(2);
constexpr int PRESSURE_DISTANCE_FACTOR = 5;
}

#if DEBUG_RENDER
#include "maths/Frustum.h"

void RenderDebugOverlay(SceneCollector& collector, const CFrustum& frustum, bool culling);

struct SDebugData {
	std::vector<SOverlaySphere> m_Spheres;
	std::vector<SOverlayLine> m_Lines;
	std::vector<SOverlayQuad> m_Quads;
} debugDataMotionMgr;
#endif

CCmpUnitMotionManager::MotionState::MotionState(ICmpPosition* cmpPos, CCmpUnitMotion* cmpMotion)
	: cmpPosition(cmpPos), cmpUnitMotion(cmpMotion)
{
	static_assert(MAX_PRESSURE <= std::numeric_limits<decltype(pushingPressure)>::max(), "MAX_PRESSURE is higher than the maximum value of the underlying type.");
}

void CCmpUnitMotionManager::ClassInit(CComponentManager& componentManager)
{
	componentManager.SubscribeToMessageType(MT_Deserialized);
	componentManager.SubscribeToMessageType(MT_TerrainChanged);
	componentManager.SubscribeToMessageType(MT_TurnStart);
	componentManager.SubscribeToMessageType(MT_Update_Final);
	componentManager.SubscribeToMessageType(MT_Update_MotionUnit);
	componentManager.SubscribeToMessageType(MT_Update_MotionFormation);
#if DEBUG_RENDER
	componentManager.SubscribeToMessageType(MT_RenderSubmit);
#endif
}

void CCmpUnitMotionManager::HandleMessage(const CMessage& msg, bool UNUSED(global))
{
	switch (msg.GetType())
	{
		case MT_TerrainChanged:
		{
			CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
			if (cmpTerrain->GetVerticesPerSide() != m_MovingUnits.width())
				ResetSubdivisions();
			break;
		}
		case MT_TurnStart:
		{
			OnTurnStart();
			break;
		}
		case MT_Update_MotionFormation:
		{
			fixed dt = static_cast<const CMessageUpdate_MotionFormation&>(msg).turnLength;
			m_ComputingMotion = true;
			MoveFormations(dt);
			m_ComputingMotion = false;
			break;
		}
		case MT_Update_MotionUnit:
		{
			fixed dt = static_cast<const CMessageUpdate_MotionUnit&>(msg).turnLength;
			m_ComputingMotion = true;
			MoveUnits(dt);
			m_ComputingMotion = false;
			break;
		}
		case MT_Deserialized:
		{
			OnDeserialized();
			break;
		}
#if DEBUG_RENDER
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderDebugOverlay(msgData.collector, msgData.frustum, msgData.culling);
			break;
		}
#endif
	}
}
void CCmpUnitMotionManager::Init(const CParamNode&)
{
	// Load some data - see CCmpPathfinder.xml.
	// This assumes the pathfinder component is initialised first and registers the validator.
	// TODO: there seems to be no real reason why we could not register a 'system' entity somewhere instead.
	CParamNode externalParamNode;
	CParamNode::LoadXML(externalParamNode, L"simulation/data/pathfinder.xml", "pathfinder");
	CParamNode pushingNode = externalParamNode.GetChild("Pathfinder").GetChild("Pushing");

	// NB: all values are given sane default, but they are not treated as optional in the schema,
	// so the XML file is the reference.

	{
		const CParamNode spread = pushingNode.GetChild("MovingSpread");
		if (spread.IsOk())
		{
			m_MovingPushingSpread = Clamp(spread.ToFixed(), entity_pos_t::Zero(), entity_pos_t::FromInt(1));
			if (m_MovingPushingSpread != spread.ToFixed())
				LOGWARNING("Moving pushing spread was clamped to the 0-1 range.");
		}
		else
			m_MovingPushingSpread = entity_pos_t::FromInt(5) / 8;
	}

	{
		const CParamNode spread = pushingNode.GetChild("StaticSpread");
		if (spread.IsOk())
		{
			m_StaticPushingSpread = Clamp(spread.ToFixed(), entity_pos_t::Zero(), entity_pos_t::FromInt(1));
			if (m_StaticPushingSpread != spread.ToFixed())
				LOGWARNING("Static pushing spread was clamped to the 0-1 range.");
		}
		else
			m_StaticPushingSpread = entity_pos_t::FromInt(5) / 8;
	}

	const CParamNode radius = pushingNode.GetChild("Radius");
	if (radius.IsOk())
	{
		m_PushingRadiusMultiplier = radius.ToFixed();
		if (m_PushingRadiusMultiplier < entity_pos_t::Zero())
		{
			LOGWARNING("Pushing radius multiplier cannot be below 0. De-activating pushing but 'pathfinder.xml' should be updated.");
			m_PushingRadiusMultiplier = entity_pos_t::Zero();
		}
		// No upper value, but things won't behave sanely if values are too high.
	}
	else
		m_PushingRadiusMultiplier = entity_pos_t::FromInt(8) / 5;

	const CParamNode minForce = pushingNode.GetChild("MinimalForce");
	if (minForce.IsOk())
		m_MinimalPushing = minForce.ToFixed();
	else
		m_MinimalPushing = entity_pos_t::FromInt(2) / 10;

	const CParamNode movingExt = pushingNode.GetChild("MovingExtension");
	const CParamNode staticExt = pushingNode.GetChild("StaticExtension");
	if (movingExt.IsOk() && staticExt.IsOk())
	{
		m_MovingPushExtension = movingExt.ToFixed();
		m_StaticPushExtension = staticExt.ToFixed();
	}
	else
	{
		m_MovingPushExtension = entity_pos_t::FromInt(5) / 2;
		m_StaticPushExtension = entity_pos_t::FromInt(2);
	}

	const CParamNode pressureStrength = pushingNode.GetChild("PressureStrength");
	if (pressureStrength.IsOk())
	{
		m_PushingPressureStrength = pressureStrength.ToFixed();
		if (m_PushingPressureStrength < entity_pos_t::Zero())
		{
			LOGWARNING("Pushing pressure strength cannot be below 0. 'pathfinder.xml' should be updated.");
			m_PushingPressureStrength = entity_pos_t::Zero();
		}
		// No upper value, but things won't behave sanely if values are too high.
	}
	else
		m_PushingPressureStrength = entity_pos_t::FromInt(1);

	const CParamNode pushingPressure = pushingNode.GetChild("PressureDecay");
	if (pushingPressure.IsOk())
	{
		m_PushingPressureDecay = Clamp(pushingPressure.ToFixed(), entity_pos_t::Zero(), entity_pos_t::FromInt(1));
		if (m_PushingPressureDecay != pushingPressure.ToFixed())
			LOGWARNING("Pushing pressure decay was clamped to the 0-1 range.");
	}
	else
		m_PushingPressureDecay = entity_pos_t::FromInt(6) / 10;

}

template<>
struct SerializeHelper<CCmpUnitMotionManager::MotionState>
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), Serialize::qualify<S, CCmpUnitMotionManager::MotionState> value)
	{
		Serializer(serialize, "pushing pressure", value.pushingPressure);
	}
};

template<>
struct SerializeHelper<EntityMap<CCmpUnitMotionManager::MotionState>>
{
	void operator()(ISerializer& serialize, const char* UNUSED(name), EntityMap<CCmpUnitMotionManager::MotionState>& value)
	{
		// Serialize manually, we don't have a default-constructor for deserialization.
		Serializer(serialize, "size", static_cast<u32>(value.size()));
		for (EntityMap<CCmpUnitMotionManager::MotionState>::iterator it = value.begin(); it != value.end(); ++it)
		{
			Serializer(serialize, "ent id", it->first);
			Serializer(serialize, "state", it->second);
		}
	}

	void operator()(IDeserializer& deserialize, const char* UNUSED(name), EntityMap<CCmpUnitMotionManager::MotionState>& value)
	{
		u32 units = 0;
		Serializer(deserialize, "size", units);
		for (u32 i = 0; i < units; ++i)
		{
			entity_id_t ent = INVALID_ENTITY;
			Serializer(deserialize, "ent id", ent);
			// Insert an invalid motion state, will be cleared up in MT_Deserialized.
			CCmpUnitMotionManager::MotionState state(nullptr, nullptr);
			Serializer(deserialize, "state", state);
			value.insert(ent, state);
		}
	}
};

void CCmpUnitMotionManager::Serialize(ISerializer& serialize)
{
	Serializer(serialize, "m_Units", m_Units);
	Serializer(serialize, "m_FormationControllers", m_FormationControllers);
}

void CCmpUnitMotionManager::Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
{
	Init(paramNode);
	ResetSubdivisions();
	Serializer(deserialize, "m_Units", m_Units);
	Serializer(deserialize, "m_FormationControllers", m_FormationControllers);
}

/**
 * This deserialization process is rather ugly, but it's required to store some data in the motion states.
 * Ideally, the motion state would actually be CCmpUnitMotion themselves, but for data locality
 * (because our components are stored randomly on the heap right now) they're not.
 * If we ever change the simulation so that components could be registered by their managers and exposed,
 * then we could just use CCmpUnitMotion directly and clean this code uglyness.
 */
void CCmpUnitMotionManager::OnDeserialized()
{
	// Fetch the components now that they exist.
	// The rest of the data was already deserialized or will be reconstructed.
	for (EntityMap<MotionState>::iterator it = m_Units.begin(); it != m_Units.end(); ++it)
	{
		it->second.cmpPosition = static_cast<ICmpPosition*>(QueryInterface(GetSimContext(), it->first, IID_Position));
		// We can know for a fact that these are CCmpUnitMotion because those are the ones registering with us
		// (and to ensure that they pass a CCmpUnitMotion pointer when registering).
		it->second.cmpUnitMotion = static_cast<CCmpUnitMotion*>(static_cast<ICmpUnitMotion*>(QueryInterface(GetSimContext(), it->first, IID_UnitMotion)));
	}
	for (EntityMap<MotionState>::iterator it = m_FormationControllers.begin(); it != m_FormationControllers.end(); ++it)
	{
		it->second.cmpPosition = static_cast<ICmpPosition*>(QueryInterface(GetSimContext(), it->first, IID_Position));
		it->second.cmpUnitMotion = static_cast<CCmpUnitMotion*>(static_cast<ICmpUnitMotion*>(QueryInterface(GetSimContext(), it->first, IID_UnitMotion)));
	}
}

void CCmpUnitMotionManager::ResetSubdivisions()
{
	CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
	if (!cmpTerrain)
		return;

	size_t size = cmpTerrain->GetMapSize();
	u16 gridSquareSize = static_cast<u16>(size / PUSHING_GRID_SIZE + 1);
	m_MovingUnits.resize(gridSquareSize, gridSquareSize);
}

void CCmpUnitMotionManager::Register(CCmpUnitMotion* component, entity_id_t ent, bool formationController)
{
	MotionState state(static_cast<ICmpPosition*>(QueryInterface(GetSimContext(), ent, IID_Position)), component);
	if (!formationController)
		m_Units.insert(ent, state);
	else
		m_FormationControllers.insert(ent, state);
}

void CCmpUnitMotionManager::Unregister(entity_id_t ent)
{
	EntityMap<MotionState>::iterator it = m_Units.find(ent);
	if (it != m_Units.end())
	{
		m_Units.erase(it);
		return;
	}
	it = m_FormationControllers.find(ent);
	if (it != m_FormationControllers.end())
		m_FormationControllers.erase(it);
}

void CCmpUnitMotionManager::OnTurnStart()
{
	for (EntityMap<MotionState>::value_type& data : m_FormationControllers)
		data.second.cmpUnitMotion->OnTurnStart();

	for (EntityMap<MotionState>::value_type& data : m_Units)
		data.second.cmpUnitMotion->OnTurnStart();
}

void CCmpUnitMotionManager::MoveUnits(fixed dt)
{
	Move(m_Units, dt);
}

void CCmpUnitMotionManager::MoveFormations(fixed dt)
{
	Move(m_FormationControllers, dt);
}

void CCmpUnitMotionManager::Move(EntityMap<MotionState>& ents, fixed dt)
{
#if DEBUG_RENDER
	debugDataMotionMgr.m_Spheres.clear();
	debugDataMotionMgr.m_Lines.clear();
	debugDataMotionMgr.m_Quads.clear();
#endif
#if DEBUG_STATS
	int comparisons = 0;
	double start = timer_Time();
#endif

	PROFILE2("MotionMgr_Move");
	std::unordered_set<std::vector<EntityMap<MotionState>::iterator>*> assigned;
	for (EntityMap<MotionState>::iterator it = ents.begin(); it != ents.end(); ++it)
	{
		if (!it->second.cmpPosition->IsInWorld())
		{
			it->second.needUpdate = false;
			continue;
		}
		else
			it->second.cmpUnitMotion->PreMove(it->second);
		it->second.initialPos = it->second.cmpPosition->GetPosition2D();
		it->second.initialAngle = it->second.cmpPosition->GetRotation().Y;
		it->second.pos = it->second.initialPos;
		it->second.speed = it->second.cmpUnitMotion->GetCurrentSpeed();
		it->second.angle = it->second.initialAngle;
		ENSURE(it->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE < m_MovingUnits.width() &&
			   it->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE < m_MovingUnits.height());
		std::vector<EntityMap<MotionState>::iterator>& subdiv = m_MovingUnits.get(
			it->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE,
			it->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE
		);
		subdiv.emplace_back(it);
		assigned.emplace(&subdiv);
	}

	for (std::vector<EntityMap<MotionState>::iterator>* vec : assigned)
	{
#if DEBUG_RENDER
		{
			SOverlayLine gridL;
			auto it = (*vec)[0];
			gridL.PushCoords(CVector3D(it->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE,
									   it->second.cmpPosition->GetHeightFixed().ToDouble() + 2.f,
									   it->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE));
			gridL.PushCoords(CVector3D(it->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE + PUSHING_GRID_SIZE,
									   it->second.cmpPosition->GetHeightFixed().ToDouble() + 2.f,
									   it->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE));
			gridL.PushCoords(CVector3D(it->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE + PUSHING_GRID_SIZE,
									   it->second.cmpPosition->GetHeightFixed().ToDouble() + 2.f,
									   it->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE + PUSHING_GRID_SIZE));
			gridL.PushCoords(CVector3D(it->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE,
									   it->second.cmpPosition->GetHeightFixed().ToDouble() + 2.f,
									   it->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE + PUSHING_GRID_SIZE));
			gridL.PushCoords(CVector3D(it->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE,
									   it->second.cmpPosition->GetHeightFixed().ToDouble() + 2.f,
									   it->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE * PUSHING_GRID_SIZE));
			gridL.m_Color = CColor(1, 1, 0, 1);
			debugDataMotionMgr.m_Lines.push_back(gridL);
		}
#endif
		for (EntityMap<MotionState>::iterator& it : *vec)
		{
			if (it->second.needUpdate)
				it->second.cmpUnitMotion->Move(it->second, dt);
			// Decay pressure after moving so we can get the full 0-MAX_PRESSURE range of values.
			it->second.pushingPressure = (m_PushingPressureDecay * it->second.pushingPressure).ToInt_RoundToZero();
		}
	}

	// Skip pushing entirely if the radius is 0
	if (&ents == &m_Units && IsPushingActivated())
	{
		PROFILE2("MotionMgr_Pushing");
		for (std::vector<EntityMap<MotionState>::iterator>* vec : assigned)
		{
			ENSURE(!vec->empty());
			std::vector< std::vector<EntityMap<MotionState>::iterator>* > consider = { vec };

			int x = (*vec)[0]->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE;
			int z = (*vec)[0]->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE;
			if (x + 1 < m_MovingUnits.width())
				consider.push_back(&m_MovingUnits.get(x + 1, z));
			if (x > 0)
				consider.push_back(&m_MovingUnits.get(x - 1, z));
			if (z + 1 < m_MovingUnits.height())
				consider.push_back(&m_MovingUnits.get(x, z + 1));
			if (z > 0)
				consider.push_back(&m_MovingUnits.get(x, z - 1));

			for (EntityMap<MotionState>::iterator& it : *vec)
			{
				if (it->second.ignore)
					continue;

#if DEBUG_RENDER
				// Plop a sphere at the unit end-pos.
				{
					SOverlaySphere sph;
					sph.m_Center = CVector3D(it->second.pos.X.ToDouble(), it->second.cmpPosition->GetHeightFixed().ToDouble() + 13.f, it->second.pos.Y.ToDouble());
					sph.m_Radius = it->second.cmpUnitMotion->m_Clearance.Multiply(PUSHING_CORRECTION).ToDouble();
					// Color the sphere: the redder, the more 'bogged down' it is.
					sph.m_Color = CColor(it->second.pushingPressure / static_cast<float>(MAX_PRESSURE), 0, 0, 1);
					debugDataMotionMgr.m_Spheres.push_back(sph);
				}
				/* Show the pushing sphere, kinda unreadable.
				{
					SOverlaySphere sph;
					sph.m_Center = CVector3D(it->second.pos.X.ToDouble(), it->second.cmpPosition->GetHeightFixed().ToDouble() + 13.f, it->second.pos.Y.ToDouble());
					sph.m_Radius = (it->second.cmpUnitMotion->m_Clearance.Multiply(PUSHING_CORRECTION).Multiply(m_PushingRadiusMultiplier) + (it->second.isMoving ? m_StaticPushExtension : m_MovingPushExtension)).ToDouble();
					// Color the sphere: the redder, the more 'bogged down' it is.
					sph.m_Color = CColor(it->second.pushingPressure / static_cast<float>(MAX_PRESSURE), 0, 0, 0.1);
					debugDataMotionMgr.m_Spheres.push_back(sph);
				}*/
				// Show the travel over this turn.
				SOverlayLine line;
				line.PushCoords(CVector3D(it->second.initialPos.X.ToDouble(),
										  it->second.cmpPosition->GetHeightFixed().ToDouble() + 13.f,
										  it->second.initialPos.Y.ToDouble()));
				line.PushCoords(CVector3D(it->second.pos.X.ToDouble(),
										  it->second.cmpPosition->GetHeightFixed().ToDouble() + 13.f,
										  it->second.pos.Y.ToDouble()));
				line.m_Color = CColor(1, 0, 1, 0.5);
				debugDataMotionMgr.m_Lines.push_back(line);
#endif
				for (std::vector<EntityMap<MotionState>::iterator>* vec2 : consider)
					for (EntityMap<MotionState>::iterator& it2 : *vec2)
						if (it->first < it2->first && !it2->second.ignore)
						{
#if DEBUG_STATS
							++comparisons;
#endif
							Push(*it, *it2, dt);
						}
			}
		}
	}

	if (IsPushingActivated())
	{
		PROFILE2("MotionMgr_PushAdjust");
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		for (std::vector<EntityMap<MotionState>::iterator>* vec : assigned)
		{
			for (EntityMap<MotionState>::iterator& it : *vec)
			{

				if (!it->second.needUpdate || it->second.ignore)
					continue;

#if DEBUG_RENDER
				SOverlayLine line;
				line.PushCoords(CVector3D(it->second.pos.X.ToDouble(),
										  it->second.cmpPosition->GetHeightFixed().ToDouble() + 15.1f ,
										  it->second.pos.Y.ToDouble()));
				line.PushCoords(CVector3D(it->second.pos.X.ToDouble() + it->second.push.X.ToDouble() * 10.f,
										  it->second.cmpPosition->GetHeightFixed().ToDouble() + 15.1f ,
										  it->second.pos.Y.ToDouble() + it->second.push.Y.ToDouble() * 10.f));
				line.m_Thickness = 0.05f;
#endif

				// Only apply pushing if the effect is significant enough.
				if (it->second.push.CompareLength(m_MinimalPushing) <= 0)
				{
#if DEBUG_RENDER
					line.m_Color = CColor(1, 1, 0, 0.6);
					debugDataMotionMgr.m_Lines.push_back(line);
#endif
					it->second.push = CFixedVector2D();
					continue;
				}

				// If there was an attempt at movement, and we're getting pushed significantly and
				// away from where we'd like to go (measured by a low dot product)
				// then mark the unit as obstructed, but push anyways.
				// (this helps units stop earlier in many situations in a realistic-ish manner).
				if (it->second.pos != it->second.initialPos
					&& (it->second.pos - it->second.initialPos).Dot(it->second.pos + it->second.push - it->second.initialPos)  < entity_pos_t::FromInt(1)/2 && it->second.pushingPressure > 30)
				{
					it->second.wasObstructed = true;
					it->second.pushingPressure = std::max<uint8_t>(MIN_PRESSURE_IF_OBSTRUCTED, it->second.pushingPressure);
					// Push anyways.
				}
#if DEBUG_RENDER
				if (it->second.wasObstructed)
					line.m_Color = CColor(1, 0, 0, 1);
				else
					line.m_Color = CColor(0, 1, 0, 1);
				debugDataMotionMgr.m_Lines.push_back(line);
#endif
				// Dampen the pushing by the current pushing pressure
				// (but prevent full dampening so that clumped units still get unclumped).
				it->second.push = it->second.push * (MAX_PRESSURE - std::min<uint8_t>(MAX_PUSH_DAMPING_PRESSURE, it->second.pushingPressure)) / MAX_PRESSURE;

				// Prevent pushed units from crossing uncrossable boundaries
				// (we can assume that normal movement didn't push units into impassable terrain).
				if ((it->second.push.X != entity_pos_t::Zero() || it->second.push.Y != entity_pos_t::Zero()) &&
					!cmpPathfinder->CheckMovement(it->second.cmpUnitMotion->GetObstructionFilter(),
						it->second.pos.X, it->second.pos.Y,
						it->second.pos.X + it->second.push.X, it->second.pos.Y + it->second.push.Y,
						it->second.cmpUnitMotion->m_Clearance,
						it->second.cmpUnitMotion->m_PassClass))
				{
					// Mark them as obstructed - this could possibly be optimised
					// perhaps it'd make more sense to mark the pushers as blocked.
					it->second.wasObstructed = true;
					it->second.wentStraight = false;
					it->second.push = CFixedVector2D();
					continue;
				}
				it->second.pos += it->second.push;
				it->second.push = CFixedVector2D();
			}
		}
	}
	{
		PROFILE2("MotionMgr_PostMove");
		for (EntityMap<MotionState>::value_type& data : ents)
		{
			if (!data.second.needUpdate)
				continue;
			data.second.cmpUnitMotion->PostMove(data.second, dt);
		}
	}
#if DEBUG_STATS
	int size = 0;
	for (std::vector<EntityMap<MotionState>::iterator>* vec : assigned)
		size += vec->size();
	double time = timer_Time() - start;
	if (comparisons > 0)
		printf(">> %i comparisons over %li grids, %f units per grid in %f secs\n", comparisons, assigned.size(), size / (float)(assigned.size()), time);
#endif
	for (std::vector<EntityMap<MotionState>::iterator>* vec : assigned)
		vec->clear();
}

// TODO: ought to better simulate in-flight pushing, e.g. if units would cross in-between turns.
void CCmpUnitMotionManager::Push(EntityMap<MotionState>::value_type& a, EntityMap<MotionState>::value_type& b, fixed dt)
{
	// The hard problem for pushing is knowing when to actually use the pathfinder to go around unpushable obstacles.
	// For simplicitly, the current logic separates moving & stopped entities:
	// moving entities will push moving entities, but not stopped ones, and vice-versa.
	// this still delivers most of the value of pushing, without a lot of the complexity.
	int movingPush = a.second.isMoving + b.second.isMoving;

	// Exception: units in the same control group (i.e. the same formation) never push farther than themselves
	// and are also allowed to push idle units (obstructions are ignored within formations,
	// so pushing idle units makes one member crossing the formation look better).
	bool sameControlGroup = a.second.controlGroup != INVALID_ENTITY && a.second.controlGroup == b.second.controlGroup;
	if (sameControlGroup)
		movingPush = 0;

	if (movingPush == 1)
		return;

	entity_pos_t combinedClearance = (a.second.cmpUnitMotion->m_Clearance + b.second.cmpUnitMotion->m_Clearance).Multiply(PUSHING_CORRECTION);
	entity_pos_t maxDist = combinedClearance;
	if (!sameControlGroup)
		maxDist = combinedClearance.Multiply(m_PushingRadiusMultiplier) + (movingPush ? m_MovingPushExtension : m_StaticPushExtension);
	combinedClearance = maxDist.Multiply(movingPush ? m_MovingPushingSpread : m_StaticPushingSpread);

	// Compare the average position of the two units over the turn - this makes overall behaviour better,
	// as we really care more about units that end up either crossing paths or staying together.
	CFixedVector2D offset = ((a.second.pos + a.second.initialPos) - (b.second.pos + b.second.initialPos)) / 2;

#if DEBUG_RENDER
	SOverlayLine line;
	line.PushCoords(CVector3D(a.second.pos.X.ToDouble(),
							  a.second.cmpPosition->GetHeightFixed().ToDouble() + 8,
							  a.second.pos.Y.ToDouble()));
	line.PushCoords(CVector3D(b.second.pos.X.ToDouble(),
							  b.second.cmpPosition->GetHeightFixed().ToDouble() + 8,
							  b.second.pos.Y.ToDouble()));
	if (offset.CompareLength(maxDist) > 0)
	{
#if DEBUG_RENDER_ALL_PUSH
		line.m_Thickness = 0.01f;
		line.m_Color = CColor(0, 0, 1, 0.4);
		debugDataMotionMgr.m_Lines.push_back(line);
		// then will return
#endif
	}
#endif
	if (offset.CompareLength(maxDist) > 0)
		return;

	entity_pos_t offsetLength;

	// If the units appear to have crossed paths, give them a strong perpendicular nudge.
	// Ideally, this will make them look like they avoided each other.
	// Worst case, either the collision detection isn't picked up or they'll end up bogged down.
	// NB: the dot product mostly works because we used average positions earlier.
	// NB: this kinda works only because our turn lengths are large enough to make this relevant.
	// In an ideal world, we'd anticipate here instead.
	// Turn it off for formations - our current 'reforming' code is bad and leads to bad behaviour.
	if (!sameControlGroup && (a.second.pos - b.second.pos).Dot(a.second.initialPos - b.second.initialPos) < PERPENDICULAR_NUDGE_THRESHOLD)
	{
		CFixedVector2D posDelta = (a.second.pos - b.second.pos) - (a.second.initialPos - b.second.initialPos);
		CFixedVector2D perp = posDelta.Perpendicular();
		// Pick the best direction to avoid the target.
		if (offset.Dot(perp) < (-offset).Dot(perp))
			offset = -perp;
		else
			offset = perp;
		offsetLength = offset.Length();
		if (offsetLength > entity_pos_t::Epsilon() * 10)
		{
			// This needs to be a strong effect or it won't really work.
			offset.X = offset.X / offsetLength * 3;
			offset.Y = offset.Y / offsetLength * 3;
		}
		offsetLength = entity_pos_t::Zero();
	}
	else
	{
		offsetLength = offset.Length();
		// If the offset is small enough that precision would be problematic, pick an arbitrary vector instead.
		if (offsetLength <= entity_pos_t::Epsilon() * 10)
		{
			// Throw in some 'randomness' so that clumped units unclump more naturaslly.
			bool dir = a.first % 2;
			offset.X = entity_pos_t::FromInt(dir ? 1 : 0);
			offset.Y = entity_pos_t::FromInt(dir ? 0 : 1);
			offsetLength = entity_pos_t::Epsilon() * 10;
		}
		else
		{
			offset.X = offset.X / offsetLength;
			offset.Y = offset.Y / offsetLength;
		}
	}

	// The pushing distance factor is 1 at the spread-modified combined clearance, >1 up to MAX if the units 'overlap', < 1 otherwise.
	entity_pos_t distanceFactor = maxDist - combinedClearance;
	// Force units that overlap a lot to have the maximum factor.
	if (distanceFactor <= entity_pos_t::Zero() || offsetLength < combinedClearance / 2)
		distanceFactor = MAX_DISTANCE_FACTOR;
	else
		distanceFactor = Clamp((maxDist - offsetLength) / distanceFactor, entity_pos_t::Zero(), MAX_DISTANCE_FACTOR);

	// Mark both as needing an update so they actually get moved.
	a.second.needUpdate = true;
	b.second.needUpdate = true;

	CFixedVector2D pushingDir = offset.Multiply(distanceFactor);

	// These cannot be zero, checked in the schema.
	entity_pos_t aWeight = a.second.cmpUnitMotion->GetWeight();
	entity_pos_t bWeight = b.second.cmpUnitMotion->GetWeight();

	// Final corrections:
	// - divide by an arbitrary constant to avoid pushing too much.
	// - multiply by the weight ratio (limiting the maximum positive push for numerical accuracy).
	entity_pos_t timeFactor = dt / PUSHING_REDUCTION_FACTOR;
	entity_pos_t maxPushing = timeFactor * MAX_PUSHING_MULTIPLIER;
	a.second.push += pushingDir.Multiply(std::min(bWeight.MulDiv(timeFactor, aWeight), maxPushing));
	b.second.push -= pushingDir.Multiply(std::min(aWeight.MulDiv(timeFactor, bWeight), maxPushing));

	// Use a constant factor to get a more general slowdown in crowded area.
	// The distance factor heavily dampens units that are overlapping.
	int addedPressure = std::max(0, (PRESSURE_STATIC_FACTOR + (distanceFactor + entity_pos_t::FromInt(-2)/3) * PRESSURE_DISTANCE_FACTOR).Multiply(m_PushingPressureStrength).ToInt_RoundToZero());
	a.second.pushingPressure = std::min(MAX_PRESSURE, a.second.pushingPressure + addedPressure);
	b.second.pushingPressure = std::min(MAX_PRESSURE, b.second.pushingPressure + addedPressure);

#if DEBUG_RENDER
	// Make the lines thicker if the force is stronger.
	line.m_Thickness = distanceFactor.ToDouble() / 10.0;
	line.m_Color = CColor(1, addedPressure / 20.f, 0, 0.8);
	debugDataMotionMgr.m_Lines.push_back(line);
#endif
}

#if DEBUG_RENDER
void RenderDebugOverlay(SceneCollector& collector, const CFrustum& frustum, bool UNUSED(culling))
{
	for (SOverlaySphere& sph: debugDataMotionMgr.m_Spheres)
		if (frustum.IsSphereVisible(sph.m_Center, sph.m_Radius))
			collector.Submit(&sph);
	for (SOverlayLine& l: debugDataMotionMgr.m_Lines)
		if (frustum.IsPointVisible(l.m_Coords[0]) || frustum.IsPointVisible(l.m_Coords[1]))
			collector.Submit(&l);
	for (SOverlayQuad& quad: debugDataMotionMgr.m_Quads)
		collector.Submit(&quad);
}
#endif

#undef DEBUG_STATS
#undef DEBUG_RENDER
#undef DEBUG_RENDER_ALL_PUSH
