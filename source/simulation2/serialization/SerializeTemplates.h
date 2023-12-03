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

#ifndef INCLUDED_SERIALIZETEMPLATES
#define INCLUDED_SERIALIZETEMPLATES

/**
 * @file
 * Helper templates definitions for serializing/deserializing common objects.
 *
 * Usage:
 * You need to (partially) specialize SerializeHelper for your type.
 * The optional SFINAE argument can be used to provide generic specializations
 * via std::enable_if_t<T>.
 * If both paths are common, you can templatize operator()'s first argument,
 * but you will need to templatize the passed value to account for different value categories.
 *
 * See SerializedTypes.h for some examples.
 *
 */

#include "simulation2/serialization/ISerializer.h"
#include "simulation2/serialization/IDeserializer.h"

// SFINAE is just there to allow SFINAE-partial specializations.
template <typename T, typename SFINAE = void>
struct SerializeHelper
{
	template<typename... Args>
	void operator()(ISerializer& serialize, const char* name, T value, Args&&...);
	template<typename... Args>
	void operator()(IDeserializer& serialize, const char* name, T& value, Args&&...);
};

// This is the variant for an explicitly specified T (where what you pass is another type).
template <typename T, typename S, typename... Args>
void Serializer(S& serialize, const char* name, Args&&... args)
{
	SerializeHelper<std::remove_const_t<std::remove_reference_t<T>>>()(serialize, name, std::forward<Args>(args)...);
}

// This lets T be deduced from the argument.
template <typename T, typename S, typename... Args>
void Serializer(S& serialize, const char* name, T&& value, Args&&... args)
{
	SerializeHelper<std::remove_const_t<std::remove_reference_t<T>>>()(serialize, name, std::forward<T>(value), std::forward<Args>(args)...);
}

namespace Serialize
{
	template<typename S, class T>
	using qualify = std::conditional_t<std::is_same_v<S, ISerializer&>, const T&, T&>;
}

#endif // INCLUDED_SERIALIZETEMPLATES
