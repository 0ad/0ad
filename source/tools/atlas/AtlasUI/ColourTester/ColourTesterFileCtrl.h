#include "VirtualDirTreeCtrl/virtualdirtreectrl.h"

// wxGenericDirCtrl could potentially be used instead of this; but it gets
// indented a long way (since its root is far further back than necessary),
// and its icons aren't very pretty, and it'd be hard to adjust it to use VFS.

class ColourTesterImageCtrl;

class ColourTesterFileCtrl : public wxVirtualDirTreeCtrl
{
public:
	ColourTesterFileCtrl(wxWindow* parent, const wxSize& size, ColourTesterImageCtrl* imgctrl);

	virtual bool OnAddDirectory(VdtcTreeItemBase &item, const wxFileName &name);

private:
	void OnSelChanged(wxTreeEvent& event);

	ColourTesterImageCtrl* m_ImageCtrl;

	DECLARE_EVENT_TABLE();
};
