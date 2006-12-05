/* Andrew Decker, a.k.a pyrolink
   Contact ajdecker1022@msn.com
   Desc: receives user input and communicates with the engine
        to perform various cinematic functions.
*/

#include "../Common/Sidebar.h"

#include "GameInterface/Messages.h"

class TrackListCtrl;
class PathListCtrl;
class NodeListCtrl;
class CinemaSliderBox;
class CinemaSpinnerBox;
class CinemaInfoBox;
class CinematicBottomBar;
class CinemaButtonBox;
class wxImage;

class CinematicSidebar : public Sidebar
{
	//For ease (from lazyness)
	friend class CinemaButtonBox;
	friend class TrackSlider;
	friend class PathSlider;
public:
	CinematicSidebar(wxWindow* sidebarContainer, wxWindow* bottomBarContainer);
	//The actual data is stored in bottom bar, but is controled from here
	void SelectTrack(ssize_t n);
	void SelectPath(ssize_t n);
	//avoid excessive shareable->vector conversion with size paramater
	void SelectSplineNode(ssize_t n, ssize_t size = -1);
	
	void AddTrack(float x, float y, float z, std::wstring& name, 
													int count);
	void AddPath(int x, int y, int z, int count);
	void AddNode(float x, float y, float z, int count);
	void UpdateTrack(std::wstring name, float timescale); 
	void UpdateTrack(float x, float y, float z);
	void UpdatePath(int x, int y, int z, ssize_t index=-1);
	void UpdateNode(float x, float y, float z, float t=-1);

	void DeleteTrack();
	void DeletePath();
	void DeleteNode();
	
	void SetSpinners(CinemaSpinnerBox* box) { m_SpinnerBox = box; }
	
	const AtlasMessage::sCinemaTrack* GetCurrentTrack();
	AtlasMessage::sCinemaPath GetCurrentPath();
	AtlasMessage::sCinemaSplineNode GetCurrentNode();
	
	int GetSelectedTrack() { return m_SelectedTrack; }
	int GetSelectedPath() { return m_SelectedPath; }
	int GetSelectedNode() { return m_SelectedSplineNode; }

	void GotoNode(ssize_t index=-1);
	void GetAbsoluteRotation(int& x, int& y, int& z, ssize_t index=-1);

	float UpdateSelectedPath();
	void UpdatePathInfo(int mode, int style, float growth, float change,
						bool drawAll, bool drawCurrent, bool drawLine);
	void UpdateSpinners();
	void UpdateTexts();
	void UpdateEngineData();

	float m_TimeElapsed;	//path time
	float m_AbsoluteTime;	//track time
	
	bool m_RotationAbsolute;	//rotation display flag in spinner box
	bool m_UpdatePathEcho;
	bool m_Playing;

protected:
	virtual void OnFirstDisplay();

private:
	// Stores all cinematics data for this map. Initialised by OnFirstDisplay.
	// Sent back to the game by [TODO]. (TODO: handle 'undo' correctly)
	std::vector<AtlasMessage::sCinemaTrack> m_Tracks;
	CinemaButtonBox* m_IconSizer;

	ssize_t m_SelectedTrack; // -1 for none
	ssize_t m_SelectedPath; // -1 for none
	ssize_t m_SelectedSplineNode; // -1 for none
	
	TrackListCtrl* m_TrackList;
	PathListCtrl* m_PathList;
	NodeListCtrl* m_NodeList;

	CinemaSliderBox* m_SliderBox;
	CinemaSpinnerBox* m_SpinnerBox; //We must update the display
	CinemaInfoBox* m_InfoBox; // ^same^
	CinematicBottomBar* m_CinemaBottomBar;

	wxImage LoadIcon(const wxString& filename);
	void LoadIcons();
};
