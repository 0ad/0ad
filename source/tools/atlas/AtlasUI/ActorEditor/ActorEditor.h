#include "Windows/AtlasWindow.h"

class ActorEditorListCtrl;

class ActorEditor : public AtlasWindow
{
	enum
	{
		ID_CreateEntity = ID_Custom
	};

public:
	ActorEditor(wxWindow* parent);

private:
	void OnCreateEntity(wxCommandEvent& event);

protected:
	AtObj FreezeData();
	void ThawData(AtObj& in);

	AtObj ExportData();
	void ImportData(AtObj& in);

	// TODO: er, what's the difference between freeze/thaw and export/import?
	// Why is the code duplicated between them?

	virtual wxString GetDefaultOpenDirectory();

private:
	ActorEditorListCtrl* m_ActorEditorListCtrl;

	wxCheckBox* m_CastShadows;
	wxCheckBox* m_Float;
	wxComboBox* m_Material;

	DECLARE_EVENT_TABLE();
};
