/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_XMBSTORAGE
#define INCLUDED_XMBSTORAGE

#include "scriptinterface/ScriptForward.h"

#include <memory>

typedef struct _xmlDoc xmlDoc;
typedef xmlDoc* xmlDocPtr;

struct IVFS;
typedef std::shared_ptr<IVFS> PIVFS;

class Path;
typedef Path VfsPath;

/**
 * Storage for XMBData
 */
class XMBStorage
{
public:
	// File headers, to make sure it doesn't try loading anything other than an XMB
	static const char* HeaderMagicStr;
	static const char* UnfinishedHeaderMagicStr;
	static const u32 XMBVersion;

	XMBStorage() = default;

	/**
	 * Read an XMB file on disk.
	 */
	bool ReadFromFile(const PIVFS& vfs, const VfsPath& filename);

	/**
	 * Parse an XML document into XMB.
	 *
	 * Main limitations:
	 *  - Can't correctly handle mixed text/elements inside elements -
	 *    "<div> <b> Text </b> </div>" and "<div> Te<b/>xt </div>" are
	 *    considered identical.
	 */
	bool LoadXMLDoc(const xmlDocPtr doc);

	/**
	 * Parse a Javascript value into XMB.
	 * The syntax is similar to ParamNode, but supports multiple children with the same name, to match XML.
	 * You need to pass the name of the root object, as unlike XML this cannot be recovered from the value.
	 * The following JS object:
	 * {
	 *     "a": 5,
	 *     "b": "test",
	 *     "x": {
	 *         // Like ParamNode, _string is used for the value.
	 *         "_string": "value",
	 *         // Like ParamNode, attributes are prefixed with @.
	 *         "@param": "something",
	 *         "y": 3
	 *     },
	 *     // Every array item is parsed as a child.
	 *     "object": [
	 *         "a",
	 *         "b",
	 *         { "_string": "c" },
	 *         { "child": "value" },
	 *     ],
	 *     // Same but without the array.
	 *     "child@0@": 1,
	 *     "child@1@": 2
	 * }
	 * will parse like the following XML:
	 * <a>5
	 *     <b>test</b>
	 *     <x param="something">value
	 *          <y>3</y>
	 *     </x>
	 *     <object>a</object>
	 *     <object>b</object>
	 *     <object>c</object>
	 *     <object><child>value</child></object>
	 *     <child>1</child>
	 *     <child>2</child>
	 * </a>
	 * See also tests for some other examples.
	 */
	bool LoadJSValue(const ScriptInterface& scriptInterface, JS::HandleValue value, const std::string& rootName);

	std::shared_ptr<u8> m_Buffer;
	size_t m_Size = 0;
};


#endif // INCLUDED_XMBSTORAGE
