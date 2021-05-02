/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_POSITION
#define INCLUDED_POSITION

#include "maths/Fixed.h"

/**
 * @file
 * Entity coordinate types
 *
 * The basic unit is the "meter".
 * To support deterministic computation across CPU architectures and compilers and
 * optimisation settings, the C++ simulation code must not use floating-point arithmetic.
 * We therefore use a fixed-point datatype for representing world positions.
 */

/**
 * Positions and distances (measured in metres)
 */
typedef CFixed_15_16 entity_pos_t;

/**
 * Rotations (measured in radians)
 */
typedef CFixed_15_16 entity_angle_t;

#endif // INCLUDED_POSITION
