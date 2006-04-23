/*
	DraggableListCtrl

	Based on wxListCtrl, but items can be reordered by dragging them around.
	Use just like a normal listctrl.
*/

#include "EditableListCtrl/EditableListCtrl.h"

class DragCommand;

class DraggableListCtrl : public EditableListCtrl
{
	friend class DragCommand;

public:
	DraggableListCtrl(wxWindow *parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxLC_ICON,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxListCtrlNameStr);

	void OnBeginDrag(wxListEvent& event);
	void OnEndDrag();

	void OnItemSelected(wxListEvent& event);
	void OnMouseEvent(wxMouseEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnMouseCaptureChanged(wxMouseCaptureChangedEvent& event);

private:
	long m_DragSource;

	DECLARE_EVENT_TABLE();
};
