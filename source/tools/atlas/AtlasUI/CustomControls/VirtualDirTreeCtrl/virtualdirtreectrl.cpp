/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/////////////////////////////////////////////////////////////////////////////
// Name:        wxVirtualDirTreeCtrl.cpp
/////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

//#ifdef __GNUG__
//    #pragma implementation "wxVirtualDirTreeCtrl.cpp"
//#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <wx/dir.h>
#include <wx/busyinfo.h>
#include "virtualdirtreectrl.h"

// default images

#include "folder.xpm"
#include "file.xpm"
#include "root.xpm"

// WDR: class implementations

//----------------------------------------------------------------------------
// wxVirtualDirTreeCtrl
//----------------------------------------------------------------------------

// WDR: event table for wxVirtualDirTreeCtrl

BEGIN_EVENT_TABLE(wxVirtualDirTreeCtrl, wxTreeCtrl)
	EVT_TREE_ITEM_EXPANDING(-1, wxVirtualDirTreeCtrl::OnExpanding)
END_EVENT_TABLE()

wxVirtualDirTreeCtrl::wxVirtualDirTreeCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
	: wxTreeCtrl(parent, id, pos, size, style, validator, name)
	, _flags(wxVDTC_DEFAULT)
{
	// create an icon list for the tree ctrl
	_iconList = new wxImageList(16,16);

	// reset to default extension list
	ResetExtensions();
}

wxVirtualDirTreeCtrl::~wxVirtualDirTreeCtrl()
{
	// first delete all VdtcTreeItemBase items (client data)
	DeleteAllItems();

	// delete the icons
	delete _iconList;
}

bool wxVirtualDirTreeCtrl::SetRootPath(const wxString &root, int flags)
{
	bool value;
	wxBusyInfo *bsy = 0;
	wxLogNull log;

	// set flags to adopt new behaviour
	_flags = flags;

	// delete all items plus root first
	DeleteAllItems();
	VdtcTreeItemBase *start = 0;

	// now call for icons management, the virtual
	// handler so the derived class can assign icons

	_iconList->RemoveAll();
	OnAssignIcons(*_iconList);

	SetImageList(_iconList);

	value = ::wxDirExists(root);
	if(value)
	{
		// call virtual handler to notify the derived class
		OnSetRootPath(root);

		// create a root item
		start = OnCreateTreeItem(VDTC_TI_ROOT, root);
		if(start)
		{
			wxFileName path;
			path.AssignDir(root);

			// call the add callback and find out if this root
			// may be added (later on)

			if(OnAddRoot(*start, path))
			{
				// add this item to the tree, with info of the developer
				wxTreeItemId id = AddRoot(start->GetCaption(), start->GetIconId(), start->GetSelectedIconId(), start);

				// show a busy dialog
				if(_flags & (wxVDTC_RELOAD_ALL | wxVDTC_SHOW_BUSYDLG))
					bsy = new wxBusyInfo(_("Please wait, scanning directory..."), 0);

				// scan directory, either the smart way or not at all
				ScanFromDir(start, path, (wxVDTC_RELOAD_ALL & _flags ? -1 : VDTC_MIN_SCANDEPTH));

				// expand root when allowed
				if(!(_flags & wxVDTC_NO_EXPAND))
					Expand(id);
			}
			else
				delete start; // sorry not succeeded
		}
	}

	// delete busy info if present
	if(bsy)
		delete bsy;

	return value;
}

int wxVirtualDirTreeCtrl::ScanFromDir(VdtcTreeItemBase *item, const wxFileName &path, int level)
{
	int value = 0;
	wxCHECK(item, -1);
	wxCHECK(item->IsDir() || item->IsRoot(), -1);

	wxLogNull log;

	// when we can still scan, do so
	if(level == -1 || level > 0)
	{
		// TODO: Maybe when a reload is issued, delete all items that are no longer present
		// in the tree (on disk) and check if all new items are present, else add them

		// if no items, then go iterate and get everything in this branch
		if(GetChildrenCount(item->GetId()) == 0)
		{
			VdtcTreeItemBaseArray addedItems;

			// now call handler, if allowed, scan this dir
			if(OnDirectoryScanBegin(path))
			{
				// get directories
				GetDirectories(item, addedItems, path);

				// get files
				if(!(_flags & wxVDTC_NO_FILES))
					GetFiles(item, addedItems, path);

				// call handler that can do a last thing
				// before sort and anything else
				OnDirectoryScanEnd(addedItems, path);

				// sort items
				if(addedItems.GetCount() > 0 && (_flags & wxVDTC_NO_SORT) == 0)
					SortItems(addedItems, 0, (int)addedItems.GetCount()-1);

				AddItemsToTreeCtrl(item, addedItems);

				// call handler to tell that the items are on the tree ctrl
				OnAddedItems(item);
			}
		}

		value = (int)GetChildrenCount(item->GetId());

		// go through all children of this node, pick out all
		// the dir classes, and descend as long as the level allows it
		// NOTE: Don't use the addedItems array, because some new can
		// be added or some can be deleted.

		wxTreeItemIdValue cookie = 0;
		VdtcTreeItemBase *b;

		wxTreeItemId child = GetFirstChild(item->GetId(), cookie);
		while(child.IsOk())
		{
			b = (VdtcTreeItemBase *)GetItemData(child);
			if(b && b->IsDir())
			{
				wxFileName tp = path;
				tp.AppendDir(b->GetName());
				value += ScanFromDir(b, tp, (level == -1 ? -1 : level-1));
			}

			child = GetNextChild(item->GetId(), cookie);
		}
	}

	return value;
}

void wxVirtualDirTreeCtrl::GetFiles(VdtcTreeItemBase *WXUNUSED(parent), VdtcTreeItemBaseArray &items, const wxFileName &path)
{
	wxFileName fpath;
	wxString fname;
	VdtcTreeItemBase *item;

	fpath = path;

	// no nodes present yet, we should start scanning this dir
	// scan files first in this directory, with all extensions in this array

	for(size_t i = 0; i < _extensions.Count(); i++)
	{
		wxDir fdir(path.GetFullPath());

		if(fdir.IsOpened())
		{
			bool bOk = fdir.GetFirst(&fname, _extensions[i], wxDIR_FILES | wxDIR_HIDDEN);
			while(bOk)
			{
				// TODO: Flag for double items

				item = AddFileItem(fname);
				if(item)
				{
					// fill it in, and marshall it by the user for info
					fpath.SetFullName(fname);
					if(OnAddFile(*item, fpath))
						items.Add(item);
					else
						delete item;
				}

				bOk = fdir.GetNext(&fname);
			}
		}
	}
}

void wxVirtualDirTreeCtrl::GetDirectories(VdtcTreeItemBase *WXUNUSED(parent), VdtcTreeItemBaseArray &items, const wxFileName &path)
{
	wxFileName fpath;
	wxString fname;
	VdtcTreeItemBase *item;

	// no nodes present yet, we should start scanning this dir
	// scan files first in this directory, with all extensions in this array

	wxDir fdir(path.GetFullPath());
	if(fdir.IsOpened())
	{
		bool bOk = fdir.GetFirst(&fname, VDTC_DIR_FILESPEC, wxDIR_DIRS | wxDIR_HIDDEN);
		while(bOk)
		{
			// TODO: Flag for double items
			item = AddDirItem(fname);
			if(item)
			{
				// fill it in, and marshall it by the user for info
				fpath = path;
				fpath.AppendDir(fname);

				if(OnAddDirectory(*item, fpath))
					items.Add(item);
				else
					delete item;
			}

			bOk = fdir.GetNext(&fname);
		}
	}
}

int wxVirtualDirTreeCtrl::OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2)
{
	// used for SortChildren, reroute to our sort routine
	VdtcTreeItemBase *a = (VdtcTreeItemBase *)GetItemData(item1),
		            *b = (VdtcTreeItemBase *)GetItemData(item2);
	if(a && b)
		return OnCompareItems(a,b);

	return 0;
}

int wxVirtualDirTreeCtrl::OnCompareItems(const VdtcTreeItemBase *a, const VdtcTreeItemBase *b)
{
	// if dir and other is not, dir has preference
	if(a->IsDir() && b->IsFile())
		return -1;
	else if(a->IsFile() && b->IsDir())
		return 1;

	// else let ascii fight it out
	return a->GetCaption().CmpNoCase(b->GetCaption());
}

void wxVirtualDirTreeCtrl::SwapItem(VdtcTreeItemBaseArray &items, int a, int b)
{
	VdtcTreeItemBase *t = items[b];
	items[b] = items[a];
	items[a] = t;
}

void wxVirtualDirTreeCtrl::SortItems(VdtcTreeItemBaseArray &items, int left, int right)
{
	VdtcTreeItemBase *a, *b;
	int i, last;

	if(left >= right)
		return;

	SwapItem(items, left, (left + right)/2);

	last = left;
	for(i = left+1; i <= right; i++)
	{
		a = items[i];
		b = items[left];
		if(a && b)
		{
			if(OnCompareItems(a, b) < 0)
				SwapItem(items, ++last, i);
		}
	}

	SwapItem(items, left, last);
	SortItems(items, left, last-1);
	SortItems(items, last+1, right);
}


void wxVirtualDirTreeCtrl::AddItemsToTreeCtrl(VdtcTreeItemBase *item, VdtcTreeItemBaseArray &items)
{
	wxCHECK2(item, return);

	// now loop through all elements on this level and add them
	// to the tree ctrl pointed out by 'id'

	VdtcTreeItemBase *t;
	wxTreeItemId id = item->GetId();
	for(size_t i = 0; i < items.GetCount(); i++)
	{
		t = items[i];
		if(t)
			AppendItem(id, t->GetCaption(), t->GetIconId(), t->GetSelectedIconId(), t);
	}
}

wxFileName wxVirtualDirTreeCtrl::GetRelativePath(const wxTreeItemId &id)
{
	wxFileName value;
	wxCHECK(id.IsOk(), value);

	VdtcTreeItemBase *b = (VdtcTreeItemBase *)GetItemData(id);
	wxCHECK(b, value);

	AppendPathRecursively(b, value, false);

	return value;
}

wxFileName wxVirtualDirTreeCtrl::GetFullPath(const wxTreeItemId &id)
{
	wxFileName value;
	wxCHECK(id.IsOk(), value);

	VdtcTreeItemBase *b = (VdtcTreeItemBase *)GetItemData(id);
	wxCHECK(b, value);

	AppendPathRecursively(b, value, true);

	return value;
}

wxTreeItemId wxVirtualDirTreeCtrl::ExpandToPath(const wxFileName &path)
{
	wxTreeItemId value((void *)0);
	wxFileName seekpath;
	wxArrayString paths;
	VdtcTreeItemBase *ptr;

	paths = path.GetDirs();

	// start in root section, and find the path sections that
	// match the sequence

	wxTreeItemId root = GetRootItem();
	if(root.IsOk())
	{
		wxTreeItemId curr = root, id;
		for(size_t i = 0; i < paths.GetCount(); i++)
		{
			// scan for name on this level of children
			wxString currpath = paths[i];
			bool not_found = true;
			wxTreeItemIdValue cookie;

			id = GetFirstChild(curr, cookie);
			while(not_found && id.IsOk())
			{
				ptr = (VdtcTreeItemBase *)GetItemData(id);
				not_found = !ptr->GetName().IsSameAs(currpath, false);

				// prevent overwriting id
				if(!not_found)
				{
					// we found the name, now to ensure there are more
					// names loaded from disk, we call ScanFromDir (it will abort anywayz
					// when there are items in the dir)

					if(ptr->IsDir())
					{
						// TODO: This getfullpath might be a too high load, we can also
						// walk along with the path, but that is a bit more tricky.
						seekpath = GetFullPath(id);
						ScanFromDir(ptr, seekpath, VDTC_MIN_SCANDEPTH);
					}

					curr = id;
				}
				else
					id = GetNextChild(curr, cookie);
			}

			// now, if not found we break out
			if(not_found)
				return value;
		}

		// when we are here we are at the final node

		Expand(curr);
		value = curr;
	}

	return value;
}

bool wxVirtualDirTreeCtrl::IsRootNode(const wxTreeItemId &id)
{
	bool value = false;
	wxCHECK(id.IsOk(), value);

	VdtcTreeItemBase *b = (VdtcTreeItemBase *)GetItemData(id);
	if(b)
		value = b->IsRoot();

	return value;
}

bool wxVirtualDirTreeCtrl::IsDirNode(const wxTreeItemId &id)
{
	bool value = false;
	wxCHECK(id.IsOk(), value);

	VdtcTreeItemBase *b = (VdtcTreeItemBase *)GetItemData(id);
	if(b)
		value = b->IsDir();

	return value;
}

bool wxVirtualDirTreeCtrl::IsFileNode(const wxTreeItemId &id)
{
	bool value = false;
	wxCHECK(id.IsOk(), value);

	VdtcTreeItemBase *b = (VdtcTreeItemBase *)GetItemData(id);
	if(b)
		value = b->IsFile();

	return value;
}

/** Appends subdirs up until root. This is done by finding the root first and
    going back down to the original caller. This is faster because no copying takes place */
void wxVirtualDirTreeCtrl::AppendPathRecursively(VdtcTreeItemBase *b, wxFileName &dir, bool useRoot)
{
	wxCHECK2(b, return);

	VdtcTreeItemBase *parent = GetParent(b);
	if(parent)
		AppendPathRecursively(parent, dir, useRoot);
	else
	{
		// no parent assume top node
		if(b->IsRoot() && useRoot)
			dir.AssignDir(b->GetName());
		return;
	}

	// now we are unwinding the other way around
	if(b->IsDir())
		dir.AppendDir(b->GetName());
	else if(b->IsFile())
		dir.SetFullName(b->GetName());
};

// -- event handlers --

void wxVirtualDirTreeCtrl::OnExpanding(wxTreeEvent &event)
{
	// check for collapsing item, and scan from there
	wxTreeItemId id = event.GetItem();
	if(id.IsOk())
	{
		VdtcTreeItemBase *t = (VdtcTreeItemBase *)GetItemData(id);
		if(t && t->IsDir())
		{
			// extract data element belonging to it, and scan.
			ScanFromDir(t, GetFullPath(id), VDTC_MIN_SCANDEPTH);

			// TODO: When this scan gives no nodes, delete all children
			// and conclude that the scan could not be performed upon expansion
		}
	}

	// be kind, and let someone else also handle this event
	event.Skip();
}

wxBitmap *wxVirtualDirTreeCtrl::CreateRootBitmap()
{
	// create root and return
	return new wxBitmap(xpm_root);
}

wxBitmap *wxVirtualDirTreeCtrl::CreateFolderBitmap()
{
	// create folder and return
	return new wxBitmap(xpm_folder);
}

wxBitmap *wxVirtualDirTreeCtrl::CreateFileBitmap()
{
	// create file and return
	return new wxBitmap(xpm_file);
}

VdtcTreeItemBase *wxVirtualDirTreeCtrl::AddFileItem(const wxString &name)
{
	// call the file item node create method
	return OnCreateTreeItem(VDTC_TI_FILE, name);
}

VdtcTreeItemBase *wxVirtualDirTreeCtrl::AddDirItem(const wxString &name)
{
	// call the dir item node create method
	return OnCreateTreeItem(VDTC_TI_DIR, name);
}


// --- virtual handlers ----

void wxVirtualDirTreeCtrl::OnAssignIcons(wxImageList &icons)
{
	wxBitmap *bmp;
	// default behaviour, assign three bitmaps

	bmp = CreateRootBitmap();
	icons.Add(*bmp);
	delete bmp;

	// 1 = folder
	bmp = CreateFolderBitmap();
	icons.Add(*bmp);
	delete bmp;

	// 2 = file
	bmp = CreateFileBitmap();
	icons.Add(*bmp);
	delete bmp;
}

VdtcTreeItemBase *wxVirtualDirTreeCtrl::OnCreateTreeItem(int type, const wxString &name)
{
	// return a default instance, no extra info needed in this item
	return new VdtcTreeItemBase(type, name);
}

bool wxVirtualDirTreeCtrl::OnAddRoot(VdtcTreeItemBase &WXUNUSED(item), const wxFileName &WXUNUSED(name))
{
	// allow adding
	return true;
}

bool wxVirtualDirTreeCtrl::OnDirectoryScanBegin(const wxFileName &WXUNUSED(path))
{
	// allow all paths
	return true;
}

bool wxVirtualDirTreeCtrl::OnAddFile(VdtcTreeItemBase &WXUNUSED(item), const wxFileName &WXUNUSED(name))
{
	// allow all files
	return true;
}

bool wxVirtualDirTreeCtrl::OnAddDirectory(VdtcTreeItemBase &WXUNUSED(item), const wxFileName &WXUNUSED(name))
{
	// allow all dirs
	return true;
}

void wxVirtualDirTreeCtrl::OnSetRootPath(const wxString &WXUNUSED(root))
{
	// do nothing here, but it can be used to start initialisation
	// based upon the setting of the root (which means a renewal from the tree)
}

void wxVirtualDirTreeCtrl::OnAddedItems(const wxTreeItemId &WXUNUSED(parent))
{
	return;
}

void wxVirtualDirTreeCtrl::OnDirectoryScanEnd(VdtcTreeItemBaseArray &WXUNUSED(items), const wxFileName &WXUNUSED(path))
{
	return;
}


