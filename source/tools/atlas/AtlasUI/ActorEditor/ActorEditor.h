#include "AtlasWindow.h"

class ActorEditorListCtrl;

class ActorEditor : public AtlasWindow
{
public:
	ActorEditor(wxWindow* parent);

protected:
	AtObj FreezeData();
	void ThawData(AtObj& in);

	AtObj ExportData();
	void ImportData(AtObj& in);

private:
	ActorEditorListCtrl* m_ActorEditorListCtrl;

	wxCheckBox* m_CastShadows;
	wxTextCtrl* m_Material;
};
