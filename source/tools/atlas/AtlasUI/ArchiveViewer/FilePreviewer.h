#include "DatafileIO/Stream/Stream.h"

class FilePreviewer : public wxPanel
{
public:
	FilePreviewer(wxWindow* parent);

	// stream can be deleted by callers after this function returns
	void PreviewFile(const wxString& filename, DatafileIO::SeekableInputStream& stream);

private:
	wxPanel* m_ContentPanel; // gets deleted and rebuilt when previewing a new file
	wxSizer* m_MainSizer;
};