#include "AtlasDialog.h"

#include "DraggableListCtrl.h"
#include "IAtlasExporter.h"

class AnimListEditorListCtrl;

//////////////////////////////////////////////////////////////////////////

class AnimListEditor : public AtlasDialog
{
	DECLARE_DYNAMIC_CLASS(AnimListEditor);

public:
	AnimListEditor();

protected:
	void Import(AtObj& in);
	void Export(AtObj& out);

private:
	AnimListEditorListCtrl* m_MainListBox;
};

//////////////////////////////////////////////////////////////////////////

class AnimListEditorListCtrl : public DraggableListCtrl, public IAtlasExporter
{
	friend class AnimListEditor;

public:
	AnimListEditorListCtrl(wxWindow* parent);

	void OnUpdate(wxCommandEvent& event);

	void Import(AtObj& in);
	void Export(AtObj& out);
};
