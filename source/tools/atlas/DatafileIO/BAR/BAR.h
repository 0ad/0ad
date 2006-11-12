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
