#include "../Common/Sidebar.h"

#include "GameInterface/Messages.h"

class CinematicSidebar : public Sidebar
{
public:
	CinematicSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

	void SelectTrack(ssize_t n);
	void SelectPath(ssize_t n);
	void SelectSplineNode(ssize_t n);

protected:
	virtual void OnFirstDisplay();

private:
	// Stores all cinematics data for this map. Initialised by OnFirstDisplay.
	// Sent back to the game by [TODO]. (TODO: handle 'undo' correctly)
	std::vector<AtlasMessage::sCinemaTrack> m_Tracks;

	ssize_t m_SelectedTrack; // -1 for none
	ssize_t m_SelectedPath; // -1 for none
	ssize_t m_SelectedSplineNode; // -1 for none

	wxListCtrl* m_TrackList;
	wxListCtrl* m_PathList;
};
