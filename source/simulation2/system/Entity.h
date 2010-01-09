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

#ifndef INCLUDED_SIM2_ENTITY
#define INCLUDED_SIM2_ENTITY
// (can't call it INCLUDED_ENTITY because that conflicts with simulation/Entity.h)

#include "lib/types.h"

/**
 * Entity ID type.
 * ID numbers are never reused within a simulation run.
 */
typedef u32 entity_id_t;

/**
 * Invalid entity ID. Used as an error return value by some functions.
 * No valid entity will have this ID.
 */
const entity_id_t INVALID_ENTITY = 0;

/**
 * Entity ID for singleton 'system' components.
 * Use with QueryInterface to get the component instance.
 * (This allows those systems to make convenient use of the common infrastructure
 * for message-passing, scripting, serialisation, etc.)
 */
const entity_id_t SYSTEM_ENTITY = 1;

#endif // INCLUDED_SIM2_ENTITY
