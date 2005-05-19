#include "AtlasWindow.h"

class ActorEditorListCtrl;

class ActorEditor : public AtlasWindow
{
public:
	ActorEditor(wxWindow* parent);

private:
	void OnCreateEntity(wxCommandEvent& event);

protected:
	AtObj FreezeData();
	void ThawData(AtObj& in);

	AtObj ExportData();
	void ImportData(AtObj& in);

	virtual wxString GetDefaultOpenDirectory();

private:
	ActorEditorListCtrl* m_ActorEditorListCtrl;

	wxCheckBox* m_CastShadows;
	wxComboBox* m_Material;

	DECLARE_EVENT_TABLE();
};
