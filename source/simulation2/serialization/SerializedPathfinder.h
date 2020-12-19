/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_SERIALIZATION_PATHFINDER
#define INCLUDED_SERIALIZATION_PATHFINDER

#include "simulation2/serialization/SerializeTemplates.h"

template<>
struct SerializeHelper<Waypoint>
{
	void operator()(ISerializer& serialize, const char* UNUSED(name), const Waypoint& value)
	{
		serialize.NumberFixed_Unbounded("waypoint x", value.x);
		serialize.NumberFixed_Unbounded("waypoint z", value.z);
	}

	void operator()(IDeserializer& deserialize, const char* UNUSED(name), Waypoint& value)
	{
		deserialize.NumberFixed_Unbounded("waypoint x", value.x);
		deserialize.NumberFixed_Unbounded("waypoint z", value.z);
	}
};

template<>
struct SerializeHelper<PathGoal>
{
	template<typename S>
	void operator()(S& serialize, const char* UNUSED(name), Serialize::qualify<S, PathGoal> value)
	{
		Serializer(serialize, "type", value.type, PathGoal::INVERTED_SQUARE);
		serialize.NumberFixed_Unbounded("goal x", value.x);
		serialize.NumberFixed_Unbounded("goal z", value.z);
		serialize.NumberFixed_Unbounded("goal u x", value.u.X);
		serialize.NumberFixed_Unbounded("goal u z", value.u.Y);
		serialize.NumberFixed_Unbounded("goal v x", value.v.X);
		serialize.NumberFixed_Unbounded("goal v z", value.v.Y);
		serialize.NumberFixed_Unbounded("goal hw", value.hw);
		serialize.NumberFixed_Unbounded("goal hh", value.hh);
		serialize.NumberFixed_Unbounded("maxdist", value.maxdist);
	}
};

#endif // INCLUDED_SERIALIZATION_PATHFINDER
