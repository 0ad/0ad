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
	class SeekableInputStream;
	class OutputStream;

	struct BAREntry
	{
		utf16string filename; // includes root name - e.g. "Data\tactics\warwagon.tactics.xmb"
		size_t filesize; // in bytes
		struct {
			unsigned short year, month, day, dayofweek; // 2005 etc, 1..12, 1..31, 0..6 (from Sunday)
			unsigned short hour, minute, second, msecond; // 1..24, 0..59, 0..59, 0..999
				// ...unless there's no date specified, in which case these will all be zero
		} modified;

	private: // implementation details
		friend class BARReader;
		size_t offset; // (assume all BARs are <4GB)
	};

	class BARReader
	{
	public:
		BARReader(SeekableInputStream& stream);

		// Read the header and file table.
		bool Initialise();

		// Get list of files.
		const std::vector<BAREntry>& GetFileList() const { return m_FileList; }

		// Get a seekable input stream for the specified file.
		// Multiple file streams can be open at the same time.
		SeekableInputStream* GetFile(const BAREntry& file) const;
		// Copy a file's contents from the archive to an output stream
		void TransferFile(const BAREntry& file, OutputStream& stream) const;

	private:
		SeekableInputStream& m_Stream;
		std::vector<BAREntry> m_FileList;

		BARReader& operator=(const BARReader&);
	};

}
