#include "wx/splitter.h"

class SnapSplitterWindow : public wxSplitterWindow
{
public:
	SnapSplitterWindow(wxWindow* parent, long style = wxSP_3D);
	void SetDefaultSashPosition(int sashPosition);
	virtual bool SplitVertically(wxWindow* window1, wxWindow* window2);
	virtual bool SplitHorizontally(wxWindow* window1, wxWindow* window2);

private:
	void OnSashPosChanging(wxSplitterEvent& evt);
	void OnDoubleClick(wxSplitterEvent& evt);

	int m_DefaultSashPosition;
	int m_SnapTolerance;

	DECLARE_EVENT_TABLE();
};
