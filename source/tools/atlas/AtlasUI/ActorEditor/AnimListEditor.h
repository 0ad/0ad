#include "Windows/AtlasDialog.h"

#include "DraggableListCtrl/DraggableListCtrl.h"

class AnimListEditorListCtrl;

//////////////////////////////////////////////////////////////////////////

class AnimListEditor : public AtlasDialog
{
	DECLARE_DYNAMIC_CLASS(AnimListEditor);

public:
	AnimListEditor();

protected:
	AtObj FreezeData();
	void ThawData(AtObj& in);

	AtObj ExportData();
	void ImportData(AtObj& in);

private:
	AnimListEditorListCtrl* m_MainListBox;
};

//////////////////////////////////////////////////////////////////////////

class AnimListEditorListCtrl : public DraggableListCtrl
{
	friend class AnimListEditor;

public:
	AnimListEditorListCtrl(wxWindow* parent);

	void OnUpdate(wxCommandEvent& event);

	void DoImport(AtObj& in);
	AtObj DoExport();
};
