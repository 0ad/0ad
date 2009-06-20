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

#include <vector>

typedef unsigned int ILuint;

namespace DatafileIO
{
	class SeekableInputStream;
	class OutputStream;

	class DDTFile
	{
	public:
		enum FileType { TGA, BMP, DDT };

		// Initialises the file, but doesn't actually read anything
		DDTFile(SeekableInputStream& stream);
		
		~DDTFile();

		// Attempts to read the file, and returns true on success
		bool Read(FileType type);

		bool SaveFile(OutputStream& stream, FileType outputType);

		// All arguments are outputs. buffer is allocated by malloc, and
		// must be freed by the caller. If realAlpha is true, the buffer
		// will be RGBA, else it'll be RGB (with height doubled and the alpha
		// stuck on the bottom)
		bool GetImageData(void*& buffer, int& width, int& height, bool realAlpha);

		enum Type_Usage {
			UNK0 = 0, // ??
			UNK1 = 1, // ??
			BUMP = 6,
			UNK2 = 7, // ??
			CUBE = 8
		};
		enum Type_Alpha {
			NONE = 0,   // ??
			PLAYER = 1, // ?? }
			TRANS = 4,  // ?? } these names are completely incorrect
			BLEND = 8,  // mostly unused
		};
		enum Type_Format {
			BGRA = 1,
			DXT1 = 4,
			GREY = 7,
			DXT3 = 8,
			NORMSPEC = 9 // DXT5, with spec in R channel, XYZ in AGB channels.
				// (See e.g. http://www.ati.com/developer/NormalMapCompression.pdf)
		};

		// (These values are not guaranteed to actually be in the enums)
		Type_Usage m_Type_Usage;
		Type_Alpha m_Type_Alpha;
		Type_Format m_Type_Format;

		int m_Type_Levels; // of mipmaps

//		struct Image
//		{
//			int width, height;
//			off_t offset;
//			size_t length;
//		};
//		std::vector<Image> m_Images;
		ILuint m_Image;

	private:
		SeekableInputStream& m_Stream;

		DDTFile& operator=(const DDTFile&);
	};
}
