#include "wx/dialog.h"

#include "AtlasWindowCommandProc.h"
#include "IAtlasExporter.h"

class FieldEditCtrl_Dialog;

class AtlasDialog : public wxDialog, public IAtlasExporter
{
	friend class FieldEditCtrl_Dialog;
	friend class AtlasWindowCommandProc;

	DECLARE_CLASS(AtlasDialog);

public:
	AtlasDialog(wxWindow* parent, const wxString& title);
	virtual ~AtlasDialog() {}

	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);

protected:
	wxPanel* m_MainPanel;

private:
	AtlasWindowCommandProc m_CommandProc;

	DECLARE_EVENT_TABLE()
};
