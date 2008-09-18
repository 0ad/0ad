#include "precompiled.h"

#include "ColourTesterFileCtrl.h"

#include "General/Datafile.h"
#include "ColourTesterImageCtrl.h"

BEGIN_EVENT_TABLE(ColourTesterFileCtrl, wxVirtualDirTreeCtrl)
	EVT_TREE_SEL_CHANGED(wxID_ANY, ColourTesterFileCtrl::OnSelChanged)
END_EVENT_TABLE()

ColourTesterFileCtrl::ColourTesterFileCtrl(wxWindow* parent, const wxSize& size, ColourTesterImageCtrl* imgctrl)
	: wxVirtualDirTreeCtrl(parent, wxID_ANY, wxDefaultPosition, size),
	m_ImageCtrl(imgctrl)
{
	wxFileName path (_T("mods/public/art/textures/skins/"));
	path.MakeAbsolute(Datafile::GetDataDirectory());
	wxASSERT(path.IsOk());
	SetRootPath(path.GetPath());
}

void ColourTesterFileCtrl::OnSelChanged(wxTreeEvent& event)
{
	if (IsFileNode(event.GetItem()))
	{
		m_ImageCtrl->SetImageFile(GetFullPath(event.GetItem()));
	}
}

bool ColourTesterFileCtrl::OnAddDirectory(VdtcTreeItemBase &item, const wxFileName &WXUNUSED(name))
{
	// Ignore .svn directories
	if (item.GetName() == _T(".svn"))
		return false;

	// Accept everything else
	return true;
}
