#include "AtlasDialog.h"

#include "DraggableListCtrl.h"
#include "IAtlasExporter.h"

class PropListEditorListCtrl;

//////////////////////////////////////////////////////////////////////////

class PropListEditor : public AtlasDialog
{
	DECLARE_DYNAMIC_CLASS(PropListEditor);

public:
	PropListEditor();

protected:
	void Import(AtObj& in);
	void Export(AtObj& out);

private:
	PropListEditorListCtrl* m_MainListBox;
};

//////////////////////////////////////////////////////////////////////////

class PropListEditorListCtrl : public DraggableListCtrl, public IAtlasExporter
{
	friend class PropListEditor;

public:
	PropListEditorListCtrl(wxWindow* parent);

	void OnUpdate(wxCommandEvent& event);

	void Import(AtObj& in);
	void Export(AtObj& out);
};
