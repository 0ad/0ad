/* Copyright (C) 2009 Wildfire Games.
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

#include <string>
#include <vector>

#include "../Util.h"

namespace DatafileIO
{
	class InputStream;
	class SeekableOutputStream;

	// Convenient storage for the internal tree
	struct XMLAttribute {
		utf16string name;
		utf16string value;
	};

	struct XMLElement {
		utf16string name;
		int linenum;
		utf16string text;
		std::vector<XMLElement*> childs;
		std::vector<XMLAttribute> attrs;
	};

	struct XMBFile;

	// Enforces initialisation/termination the XML system
	struct XMLReader {
		XMLReader();
		~XMLReader();

		XMBFile* LoadFromXML(InputStream& stream);
	};

	struct XMBFile {
		XMLElement* root;
		enum { UNKNOWN, AOM, AOE3 } format;

		XMBFile();
		~XMBFile();

		// This does *not* understand l33t-compressed data. Please
		// decompress them first.
		static XMBFile* LoadFromXMB(InputStream& stream);

		// Caller should convert the returned data to some encoding, and
		// prepend "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" (or whatever
		// is appropriate for that encoding)
		std::wstring SaveAsXML();

		void SaveAsXMB(SeekableOutputStream& stream);
	};
};
