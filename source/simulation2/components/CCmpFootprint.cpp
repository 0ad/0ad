/* Copyright (C) 2010 Wildfire Games.
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

#include "ICmpObstruction.h"
#include "ICmpObstructionManager.h"
#include "ICmpPosition.h"
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

	static std::string GetSchema()
	{
		return
			"<a:help>Approximation of the entity's shape, for collision detection and outline rendering. "
			"Shapes are flat horizontal squares or circles, extended vertically to a given height.</a:help>"
			"<a:example>"
				"<Square width='3.0' height='3.0'/>"
				"<Height>0.0</Height>"
			"</a:example>"
			"<a:example>"
				"<Circle radius='0.5'/>"
				"<Height>0.0</Height>"
			"</a:example>"
			"<choice>"
				"<element name='Square' a:help='Set the footprint to a square of the given size'>"
					"<attribute name='width' a:help='Size of the footprint along the left/right direction (in metres)'>"
						"<ref name='positiveDecimal'/>"
					"</attribute>"
					"<attribute name='depth' a:help='Size of the footprint along the front/back direction (in metres)'>"
						"<ref name='positiveDecimal'/>"
					"</attribute>"
				"</element>"
				"<element name='Circle' a:help='Set the footprint to a circle of the given size'>"
					"<attribute name='radius' a:help='Radius of the footprint (in metres)'>"
						"<ref name='positiveDecimal'/>"
					"</attribute>"
				"</element>"
			"</choice>"
			"<element name='Height' a:help='Vertical extent of the footprint (in metres)'>"
				"<ref name='nonNegativeDecimal'/>"
			"</element>";
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

	virtual void GetShape(EShape& shape, entity_pos_t& size0, entity_pos_t& size1, entity_pos_t& height)
	{
		shape = m_Shape;
		size0 = m_Size0;
		size1 = m_Size1;
		height = m_Height;
	}

	virtual CFixedVector3D PickSpawnPoint(entity_id_t spawned)
	{
		// Try to find a free space around the building's footprint.
		// (Note that we use the footprint, not the obstruction shape - this might be a bit dodgy
		// because the footprint might be inside the obstruction, but it hopefully gives us a nicer
		// shape.)

		CFixedVector3D error(fixed::FromInt(-1), fixed::FromInt(-1), fixed::FromInt(-1));

		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null() || !cmpPosition->IsInWorld())
			return error;

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
		if (cmpObstructionManager.null())
			return error;

		entity_pos_t spawnedRadius;
		ICmpObstructionManager::tag_t spawnedTag;

		CmpPtr<ICmpObstruction> cmpSpawnedObstruction(GetSimContext(), spawned);
		if (!cmpSpawnedObstruction.null())
		{
			spawnedRadius = cmpSpawnedObstruction->GetUnitRadius();
			spawnedTag = cmpSpawnedObstruction->GetObstruction();
		}
		// else use zero radius

		// The spawn point should be far enough from this footprint to fit the unit, plus a little gap
		entity_pos_t clearance = spawnedRadius + entity_pos_t::FromInt(2);

		CFixedVector2D initialPos = cmpPosition->GetPosition2D();
		entity_angle_t initialAngle = cmpPosition->GetRotation().Y;

		if (m_Shape == CIRCLE)
		{
			entity_pos_t radius = m_Size0 + clearance;

			// Try equally-spaced points around the circle, starting from the front and expanding outwards in alternating directions
			const ssize_t numPoints = 31;
			for (ssize_t i = 0; i < (numPoints+1)/2; i = (i > 0 ? -i : 1-i)) // [0, +1, -1, +2, -2, ... (np-1)/2, -(np-1)/2]
			{
				entity_angle_t angle = initialAngle + (entity_angle_t::Pi()*2).Multiply(entity_angle_t::FromInt(i)/(int)numPoints);

				fixed s, c;
				sincos_approx(angle, s, c);

				CFixedVector3D pos (initialPos.X + s.Multiply(radius), fixed::Zero(), initialPos.Y + c.Multiply(radius));

				SkipTagObstructionFilter filter(spawnedTag); // ignore collisions with the spawned entity
				if (!cmpObstructionManager->TestUnitShape(filter, pos.X, pos.Z, spawnedRadius))
					return pos; // this position is okay, so return it
			}
		}
		else
		{
			fixed s, c;
			sincos_approx(initialAngle, s, c);

			for (size_t edge = 0; edge < 4; ++edge)
			{
				// Try equally-spaced points along the edge, starting from the middle and expanding outwards in alternating directions
				const ssize_t numPoints = 9;

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
				CFixedVector2D center = initialPos - dir.Perpendicular().Multiply(sy/2 + clearance);
				dir = dir.Multiply((sx + clearance*2) / (int)(numPoints-1));

				for (ssize_t i = 0; i < (numPoints+1)/2; i = (i > 0 ? -i : 1-i)) // [0, +1, -1, +2, -2, ... (np-1)/2, -(np-1)/2]
				{
					CFixedVector2D pos (center + dir*i);

					SkipTagObstructionFilter filter(spawnedTag); // ignore collisions with the spawned entity
					if (!cmpObstructionManager->TestUnitShape(filter, pos.X, pos.Y, spawnedRadius))
						return CFixedVector3D(pos.X, fixed::Zero(), pos.Y); // this position is okay, so return it
				}
			}
		}

		return error;
	}
};

REGISTER_COMPONENT_TYPE(Footprint)
