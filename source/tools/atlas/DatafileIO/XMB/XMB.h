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
