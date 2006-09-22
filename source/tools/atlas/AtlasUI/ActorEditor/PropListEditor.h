#include "Windows/AtlasDialog.h"

#include "DraggableListCtrl/DraggableListCtrl.h"

class PropListEditorListCtrl;

//////////////////////////////////////////////////////////////////////////

class PropListEditor : public AtlasDialog
{
public:
	PropListEditor(wxWindow* parent);
	static AtlasDialog* Create(wxWindow* parent) { return new PropListEditor(parent); }

protected:
	AtObj FreezeData();
	void ThawData(AtObj& in);

	AtObj ExportData();
	void ImportData(AtObj& in);

private:
	PropListEditorListCtrl* m_MainListBox;
};

//////////////////////////////////////////////////////////////////////////

class PropListEditorListCtrl : public DraggableListCtrl
{
	friend class PropListEditor;

public:
	PropListEditorListCtrl(wxWindow* parent);

	void OnUpdate(wxCommandEvent& event);

	void DoImport(AtObj& in);
	AtObj DoExport();
};
