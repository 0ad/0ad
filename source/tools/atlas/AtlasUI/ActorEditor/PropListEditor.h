#include "AtlasDialog.h"

#include "DraggableListCtrl.h"

class PropListEditorListCtrl;

//////////////////////////////////////////////////////////////////////////

class PropListEditor : public AtlasDialog
{
	DECLARE_DYNAMIC_CLASS(PropListEditor);

public:
	PropListEditor();

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
