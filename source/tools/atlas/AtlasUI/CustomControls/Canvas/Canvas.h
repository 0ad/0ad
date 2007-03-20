#include "wx/glcanvas.h"

class Canvas : public wxGLCanvas
{
public:
	Canvas(wxWindow* parent, int* attribList, long style);

	void InitSize();
protected:
	virtual void HandleMouseEvent(wxMouseEvent& evt) = 0;

private:
	void OnResize(wxSizeEvent& evt);
	void OnMouseCapture(wxMouseCaptureChangedEvent& evt);
	void OnMouse(wxMouseEvent& evt);

	bool m_SuppressResize;

	wxPoint m_LastMousePos;
	bool m_MouseCaptured;

	DECLARE_EVENT_TABLE();
};
