#include "DraggableListCtrl.h"

#include "IAtlasExporter.h"

class ActorEditor;

class ActorEditorListCtrl : public DraggableListCtrl, public IAtlasExporter
{
	friend class ActorEditor;

public:
	ActorEditorListCtrl(wxWindow* parent);

	void OnUpdate(wxCommandEvent& event);

private:
	void Import(AtObj& in);
	void Export(AtObj& out);
};