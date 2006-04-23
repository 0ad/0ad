#include "wx/dialog.h"

#include "General/AtlasWindowCommandProc.h"
#include "General/IAtlasSerialiser.h"

class FieldEditCtrl_Dialog;

class AtlasDialog : public wxDialog, public IAtlasSerialiser
{
	friend class FieldEditCtrl_Dialog;
	friend class AtlasWindowCommandProc;

	DECLARE_CLASS(AtlasDialog);

public:
	AtlasDialog(wxWindow* parent, const wxString& title, const wxSize& size);
	virtual ~AtlasDialog() {}

private:
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);

protected:
	wxPanel* m_MainPanel;

private:
	AtlasWindowCommandProc m_CommandProc;

	DECLARE_EVENT_TABLE()
};
