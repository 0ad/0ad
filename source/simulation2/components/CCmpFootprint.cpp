/* Copyright (C) 2018 Wildfire Games.
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
#include "ICmpFootprint.h"

#include "ps/Profile.h"
#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpPathfinder.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpRallyPoint.h"
#include "simulation2/components/ICmpUnitMotion.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/MessageTypes.h"
#include "maths/FixedVector2D.h"

class CCmpFootprint : public ICmpFootprint
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(Footprint)

	EShape m_Shape;
	entity_pos_t m_Size0; // width/radius
	entity_pos_t m_Size1; // height/radius
	entity_pos_t m_Height;
	entity_pos_t m_MaxSpawnDistance;

	static std::string GetSchema()
	{
		return
			"<a:help>Approximation of the entity's shape, for collision detection and outline rendering. "
			"Shapes are flat horizontal squares or circles, extended vertically to a given height.</a:help>"
			"<a:example>"
				"<Square width='3.0' height='3.0'/>"
				"<Height>0.0</Height>"
				"<MaxSpawnDistance>8</MaxSpawnDistance>"
			"</a:example>"
			"<a:example>"
				"<Circle radius='0.5'/>"
				"<Height>0.0</Height>"
				"<MaxSpawnDistance>8</MaxSpawnDistance>"
			"</a:example>"
			"<choice>"
				"<element name='Square' a:help='Set the footprint to a square of the given size'>"
					"<attribute name='width' a:help='Size of the footprint along the left/right direction (in metres)'>"
						"<data type='decimal'>"
							"<param name='minExclusive'>0.0</param>"
						"</data>"
					"</attribute>"
					"<attribute name='depth' a:help='Size of the footprint along the front/back direction (in metres)'>"
						"<data type='decimal'>"
							"<param name='minExclusive'>0.0</param>"
						"</data>"
					"</attribute>"
				"</element>"
				"<element name='Circle' a:help='Set the footprint to a circle of the given size'>"
					"<attribute name='radius' a:help='Radius of the footprint (in metres)'>"
						"<data type='decimal'>"
							"<param name='minExclusive'>0.0</param>"
						"</data>"
					"</attribute>"
				"</element>"
			"</choice>"
			"<element name='Height' a:help='Vertical extent of the footprint (in metres)'>"
				"<ref name='nonNegativeDecimal'/>"
			"</element>"
			"<optional>"
				"<element name='MaxSpawnDistance' a:help='Farthest distance units can spawn away from the edge of this entity'>"
					"<ref name='nonNegativeDecimal'/>"
				"</element>"
			"</optional>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		if (paramNode.GetChild("Square").IsOk())
		{
			m_Shape = SQUARE;
			m_Size0 = paramNode.GetChild("Square").GetChild("@width").ToFixed();
			m_Size1 = paramNode.GetChild("Square").GetChild("@depth").ToFixed();
		}
		else if (paramNode.GetChild("Circle").IsOk())
		{
			m_Shape = CIRCLE;
			m_Size0 = m_Size1 = paramNode.GetChild("Circle").GetChild("@radius").ToFixed();
		}
		else
		{
			// Error - pick some default
			m_Shape = CIRCLE;
			m_Size0 = m_Size1 = entity_pos_t::FromInt(1);
		}

		m_Height = paramNode.GetChild("Height").ToFixed();

		if (paramNode.GetChild("MaxSpawnDistance").IsOk())
			m_MaxSpawnDistance = paramNode.GetChild("MaxSpawnDistance").ToFixed();
		else
			// Pick some default
			m_MaxSpawnDistance = entity_pos_t::FromInt(7);
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// No dynamic state to serialize
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual void GetShape(EShape& shape, entity_pos_t& size0, entity_pos_t& size1, entity_pos_t& height) const
	{
		shape = m_Shape;
		size0 = m_Size0;
		size1 = m_Size1;
		height = m_Height;
	}

	virtual CFixedVector3D PickSpawnPoint(entity_id_t spawned) const
	{
		PROFILE3("PickSpawnPoint");

		// Try to find a free space around the building's footprint.
		// (Note that we use the footprint, not the obstruction shape - this might be a bit dodgy
		// because the footprint might be inside the obstruction, but it hopefully gives us a nicer
		// shape.)

		const CFixedVector3D error(fixed::FromInt(-1), fixed::FromInt(-1), fixed::FromInt(-1));

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return error;

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (!cmpObstructionManager)
			return error;

		// If no spawned obstruction, use a positive radius to avoid division by zero errors.
		entity_pos_t spawnedRadius = fixed::FromInt(1);
		ICmpObstructionManager::tag_t spawnedTag;

		CmpPtr<ICmpObstruction> cmpSpawnedObstruction(GetSimContext(), spawned);
		if (cmpSpawnedObstruction)
		{
			spawnedRadius = cmpSpawnedObstruction->GetUnitRadius();
			spawnedTag = cmpSpawnedObstruction->GetObstruction();
		}

		// Get passability class from UnitMotion.
		CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetSimContext(), spawned);
		if (!cmpUnitMotion)
			return error;

		pass_class_t spawnedPass = cmpUnitMotion->GetPassabilityClass();
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (!cmpPathfinder)
			return error;

		// Ignore collisions with the spawned entity and entities that don't block movement.
		SkipTagRequireFlagsObstructionFilter filter(spawnedTag, ICmpObstructionManager::FLAG_BLOCK_MOVEMENT);

		CFixedVector2D initialPos = cmpPosition->GetPosition2D();
		entity_angle_t initialAngle = cmpPosition->GetRotation().Y;

		CFixedVector2D u = CFixedVector2D(fixed::Zero(), fixed::FromInt(1)).Rotate(initialAngle);
		CFixedVector2D v = u.Perpendicular();

		// Obstructions are squares, so multiply its radius by 2*sqrt(2) ~= 3 to determine the distance between units.
		entity_pos_t gap = spawnedRadius * 3;
		int rows = std::max(1, (m_MaxSpawnDistance / gap).ToInt_RoundToInfinity());

		// The first row of units will be half a gap away from the footprint.
		CFixedVector2D halfSize = m_Shape == CIRCLE ?
			CFixedVector2D(m_Size1 + gap / 2, m_Size0 + gap / 2) :
			CFixedVector2D((m_Size1 + gap) / 2, (m_Size0 + gap) / 2);

		// Figure out how many units can fit on each halfside of the rectangle.
		// Since 2*pi/6 ~= 1, this is also how many units can fit on a sixth of the circle.
		int distX = std::max(1, (halfSize.X / gap).ToInt_RoundToNegInfinity());
		int distY = std::max(1, (halfSize.Y / gap).ToInt_RoundToNegInfinity());

		// Try more spawning points for large units in case some of them are partially blocked.
		if (rows == 1)
		{
			distX *= 2;
			distY *= 2;
		}

		// Store the position of the spawning point within each row that's closest to the spawning angle.
		std::vector<int> offsetPoints(rows, 0);

		CmpPtr<ICmpRallyPoint> cmpRallyPoint(GetEntityHandle());
		if (cmpRallyPoint && cmpRallyPoint->HasPositions())
		{
			CFixedVector2D rallyPointPos = cmpRallyPoint->GetFirstPosition();
			if (m_Shape == CIRCLE)
			{
				entity_angle_t offsetAngle = atan2_approx(rallyPointPos.X - initialPos.X, rallyPointPos.Y - initialPos.Y) - initialAngle;

				// There are 6*(distX+r) points in row r, so multiply that by angle/2pi to find the offset within the row.
				for (int r = 0; r < rows; ++r)
					offsetPoints[r] = (offsetAngle * 3 * (distX + r) / fixed::Pi()).ToInt_RoundToNearest();
			}
			else
			{
				CFixedVector2D offsetPos = Geometry::NearestPointOnSquare(rallyPointPos - initialPos, u, v, halfSize);

				// Scale and convert the perimeter coordinates of the point to its offset within the row.
				int x = (offsetPos.Dot(u) * distX / halfSize.X).ToInt_RoundToNearest();
				int y = (offsetPos.Dot(v) * distY / halfSize.Y).ToInt_RoundToNearest();
				for (int r = 0; r < rows; ++r)
					offsetPoints[r] = Geometry::GetPerimeterDistance(
						distX + r,
						distY + r,
						x >= distX ? distX + r : x <= -distX ? -distX - r : x,
						y >= distY ? distY + r : y <= -distY ? -distY - r : y);
			}
		}

		for (int k = 0; k < 2 * (distX + distY + 2 * rows); k = k > 0 ? -k : 1 - k)
			for (int r = 0; r < rows; ++r)
			{
				CFixedVector2D pos = initialPos;
				if (m_Shape == CIRCLE)
					// Multiply the point by 2pi / 6*(distX+r) to get the angle.
					pos += u.Rotate(fixed::Pi() * (offsetPoints[r] + k) / (3 * (distX + r))).Multiply(halfSize.X + gap * r );
				else
				{
					// Convert the point to coordinates and scale.
					std::pair<int, int> p = Geometry::GetPerimeterCoordinates(distX + r, distY + r, offsetPoints[r] + k);
					pos += u.Multiply((halfSize.X + gap * r) * p.first / (distX + r)) +
					       v.Multiply((halfSize.Y + gap * r) * p.second / (distY + r));
				}

				if (cmpPathfinder->CheckUnitPlacement(filter, pos.X, pos.Y, spawnedRadius, spawnedPass) == ICmpObstruction::FOUNDATION_CHECK_SUCCESS)
					return CFixedVector3D(pos.X, fixed::Zero(), pos.Y);
			}

		return error;
	}

	virtual CFixedVector3D PickSpawnPointBothPass(entity_id_t spawned) const
	{
		PROFILE3("PickSpawnPointBothPass");

		// Try to find a free space inside and around this footprint
		// at the intersection between the footprint passability and the unit passability.
		// (useful for example for destroyed ships where the spawning point should be in the intersection
		// of the unit and ship passabilities).
		// As the overlap between these passabilities regions may be narrow, we need a small step (1 meter)

		const CFixedVector3D error(fixed::FromInt(-1), fixed::FromInt(-1), fixed::FromInt(-1));

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return error;

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (!cmpObstructionManager)
			return error;

		entity_pos_t spawnedRadius;
		ICmpObstructionManager::tag_t spawnedTag;

		CmpPtr<ICmpObstruction> cmpSpawnedObstruction(GetSimContext(), spawned);
		if (cmpSpawnedObstruction)
		{
			spawnedRadius = cmpSpawnedObstruction->GetUnitRadius();
			spawnedTag = cmpSpawnedObstruction->GetObstruction();
		}
		// else use zero radius

		// Get passability class from UnitMotion
		CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetSimContext(), spawned);
		if (!cmpUnitMotion)
			return error;

		pass_class_t spawnedPass = cmpUnitMotion->GetPassabilityClass();
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (!cmpPathfinder)
			return error;

		// Get the Footprint entity passability
		CmpPtr<ICmpUnitMotion> cmpEntityMotion(GetEntityHandle());
		if (!cmpEntityMotion)
			return error;
		pass_class_t entityPass = cmpEntityMotion->GetPassabilityClass();

		CFixedVector2D initialPos = cmpPosition->GetPosition2D();
		entity_angle_t initialAngle = cmpPosition->GetRotation().Y;

		// Max spawning distance + 1 (in meters)
		const i32 maxSpawningDistance = 13;

		if (m_Shape == CIRCLE)
		{
			// Expand outwards from foundation with a fixed step of 1 meter
			for (i32 dist = 0; dist <= maxSpawningDistance; ++dist)
			{
				// The spawn point should be far enough from this footprint to fit the unit, plus a little gap
				entity_pos_t clearance = spawnedRadius + entity_pos_t::FromInt(1+dist);
				entity_pos_t radius = m_Size0 + clearance;

				// Try equally-spaced points around the circle in alternating directions, starting from the front
				const i32 numPoints = 31 + 2*dist;
				for (i32 i = 0; i < (numPoints+1)/2; i = (i > 0 ? -i : 1-i)) // [0, +1, -1, +2, -2, ... (np-1)/2, -(np-1)/2]
				{
					entity_angle_t angle = initialAngle + (entity_angle_t::Pi()*2).Multiply(entity_angle_t::FromInt(i)/(int)numPoints);

					fixed s, c;
					sincos_approx(angle, s, c);

					CFixedVector3D pos (initialPos.X + s.Multiply(radius), fixed::Zero(), initialPos.Y + c.Multiply(radius));

					SkipTagObstructionFilter filter(spawnedTag); // ignore collisions with the spawned entity
					if (cmpPathfinder->CheckUnitPlacement(filter, pos.X, pos.Z, spawnedRadius, spawnedPass) == ICmpObstruction::FOUNDATION_CHECK_SUCCESS &&
						cmpPathfinder->CheckUnitPlacement(filter, pos.X, pos.Z, spawnedRadius, entityPass) == ICmpObstruction::FOUNDATION_CHECK_SUCCESS)
						return pos; // this position is okay, so return it
				}
			}
		}
		else
		{
			fixed s, c;
			sincos_approx(initialAngle, s, c);

			// Expand outwards from foundation with a fixed step of 1 meter
			for (i32 dist = 0; dist <= maxSpawningDistance; ++dist)
			{
				// The spawn point should be far enough from this footprint to fit the unit, plus a little gap
				entity_pos_t clearance = spawnedRadius + entity_pos_t::FromInt(1+dist);

				for (i32 edge = 0; edge < 4; ++edge)
				{
					// Compute the direction and length of the current edge
					CFixedVector2D dir;
					fixed sx, sy;
					switch (edge)
					{
					case 0:
						dir = CFixedVector2D(c, -s);
						sx = m_Size0;
						sy = m_Size1;
						break;
					case 1:
						dir = CFixedVector2D(-s, -c);
						sx = m_Size1;
						sy = m_Size0;
						break;
					case 2:
						dir = CFixedVector2D(s, c);
						sx = m_Size1;
						sy = m_Size0;
						break;
					case 3:
						dir = CFixedVector2D(-c, s);
						sx = m_Size0;
						sy = m_Size1;
						break;
					}
					sx = sx/2 + clearance;
					sy = sy/2 + clearance;
					// Try equally-spaced (1 meter) points along the edge in alternating directions, starting from the middle
					i32 numPoints = 1 + 2*sx.ToInt_RoundToNearest();
					CFixedVector2D center = initialPos - dir.Perpendicular().Multiply(sy);
					for (i32 i = 0; i < (numPoints+1)/2; i = (i > 0 ? -i : 1-i)) // [0, +1, -1, +2, -2, ... (np-1)/2, -(np-1)/2]
					{
						CFixedVector2D pos (center + dir*i);

						SkipTagObstructionFilter filter(spawnedTag); // ignore collisions with the spawned entity
						if (cmpPathfinder->CheckUnitPlacement(filter, pos.X, pos.Y, spawnedRadius, spawnedPass) == ICmpObstruction::FOUNDATION_CHECK_SUCCESS &&
							cmpPathfinder->CheckUnitPlacement(filter, pos.X, pos.Y, spawnedRadius, entityPass) == ICmpObstruction::FOUNDATION_CHECK_SUCCESS)
       							return CFixedVector3D(pos.X, fixed::Zero(), pos.Y); // this position is okay, so return it
					}
				}
			}
		}

		return error;
	}
};

REGISTER_COMPONENT_TYPE(Footprint)
