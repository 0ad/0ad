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

#ifndef INCLUDED_SCRIPTINTERFACE_JSON
#define INCLUDED_SCRIPTINTERFACE_JSON

#include "ScriptForward.h"

#include <string>

class Path;
using VfsPath = Path;
class ScriptRequest;

/**
 * @file JSON.h
 * Contains JSON and more generally object-string conversion functions.
 */

namespace Script
{
/**
 * Convert an object to a UTF-8 encoded string, either with JSON
 * (if pretty == true and there is no JSON error) or with toSource().
 */
std::string ToString(const ScriptRequest& rq, JS::MutableHandleValue obj, bool pretty = false);

/**
 * Parse a UTF-8-encoded JSON string. Returns the unmodified value on error
 * and prints an error message.
 * @return true on success; false otherwise
 */
bool ParseJSON(const ScriptRequest& rq, const std::string& string_utf8, JS::MutableHandleValue out);

/**
 * Read a JSON file. Returns the unmodified value on error and prints an error message.
 */
void ReadJSONFile(const ScriptRequest& rq, const VfsPath& path, JS::MutableHandleValue out);

/**
 * Stringify to a JSON string, UTF-8 encoded. Returns an empty string on error.
 */
std::string StringifyJSON(const ScriptRequest& rq, JS::MutableHandleValue obj, bool indent = true);
}

#endif // INCLUDED_SCRIPTINTERFACE_JSON
