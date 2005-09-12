#include "wx/splitter.h"

class SnapSplitterWindow : public wxSplitterWindow
{
public:
	SnapSplitterWindow(wxWindow* parent, long style = wxSP_3D);
	virtual bool SplitVertically(wxWindow *window1, wxWindow *window2, int sashPosition);
	virtual bool SplitHorizontally(wxWindow *window1, wxWindow *window2, int sashPosition);

private:
	void OnSashPosChanging(wxSplitterEvent& evt);

	int m_DefaultSashPosition;
	int m_SnapTolerance;

	DECLARE_EVENT_TABLE();
};
