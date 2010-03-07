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

#ifndef INCLUDED_ICMPPROJECTILEMANAGER
#define INCLUDED_ICMPPROJECTILEMANAGER

#include "simulation2/system/Interface.h"

#include "maths/Fixed.h"
#include "maths/FixedVector3D.h"

/**
 * Projectile manager. This deals with the rendering and the graphical motion of projectiles.
 * (The gameplay effects of projectiles are not handled here - the simulation code does that
 * with timers, this just does the visual aspects, because it's simpler to keep the parts separated.)
 */
class ICmpProjectileManager : public IComponent
{
public:
	/**
	 * Launch a projectile from entity @p source to entity @p target.
	 * @param source source entity; the projectile will determined from the "projectile" prop in its actor
	 * @param target target entity; the projectile will automatically track the target to ensure it always hits precisely
	 * @param speed horizontal speed in m/s
	 * @param gravity gravitational acceleration in m/s^2 (determines the height of the ballistic curve)
	 */
	virtual void LaunchProjectileAtEntity(entity_id_t source, entity_id_t target, CFixed_23_8 speed, CFixed_23_8 gravity) = 0;

	/**
	 * Launch a projectile from entity @p source to point @p target.
	 * @param source source entity; the projectile will determined from the "projectile" prop in its actor
	 * @param target target point
	 * @param speed horizontal speed in m/s
	 * @param gravity gravitational acceleration in m/s^2 (determines the height of the ballistic curve)
	 */
	virtual void LaunchProjectileAtPoint(entity_id_t source, CFixedVector3D target, CFixed_23_8 speed, CFixed_23_8 gravity) = 0;

	DECLARE_INTERFACE_TYPE(ProjectileManager)
};

#endif // INCLUDED_ICMPPROJECTILEMANAGER
