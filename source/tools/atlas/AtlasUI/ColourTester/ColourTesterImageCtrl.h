#include "wx/image.h"

class ColourTesterImageCtrl : public wxWindow
{
public:
	ColourTesterImageCtrl(wxWindow* parent);

	void SetImageFile(const wxFileName& fn);
	void SetColour(const wxColour& colour);
	void SetZoom(int amount);

	void OnPaint(wxPaintEvent& event);

private:
	void CalculateImage();

	bool m_Valid; // stores whether a valid image is loaded and displayable
	unsigned int m_OriginalImage; // DevIL image id
	wxImage m_FinalImage;
	wxBitmap m_FinalBitmap;

	unsigned char m_Colour[3]; // RGB bytes

	int m_ZoomAmount;

	DECLARE_EVENT_TABLE();
};
