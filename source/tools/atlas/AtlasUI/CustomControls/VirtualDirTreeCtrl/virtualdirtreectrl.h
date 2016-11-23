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
// Name:        wxVirtualDirTreeCtrl.h
/////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_VIRTUALDIRTREECTRL
#define INCLUDED_VIRTUALDIRTREECTRL

//#ifdef __GNUG__
//    #pragma interface "virtualdirtreectrl.cpp"
//#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/dynarray.h>
#include <wx/treectrl.h>
#include <wx/filesys.h>
#include <wx/imaglist.h>

enum
{
	VDTC_TI_ROOT = 0,
	VDTC_TI_DIR,
	VDTC_TI_FILE
};

#ifndef LINUX
  #define VDTC_DIR_FILESPEC _T("*.*")
#else
  #define VDTC_DIR_FILESPEC _T("*")
#endif

/// Icon number for root
#define VDTC_ICON_ROOT     0

/// Icon number for a folder
#define VDTC_ICON_DIR      1

/// Icon number for file
#define VDTC_ICON_FILE     2

/// Minimal amount of levels to scan per run
#define VDTC_MIN_SCANDEPTH 2

/// No settings, take default
#define wxVDTC_DEFAULT      0
/** Instead of performing a smart reload per collapsed node, all items are loaded
    into memory. NOTE: This can take a while! */
#define wxVDTC_RELOAD_ALL   1
/** Show a busy dialog to inform user, only valid when wxVDTC_RELOAD_ALL is set. Showing
    a busy dialog on every smart expansion is not useful anyway */
#define wxVDTC_SHOW_BUSYDLG 2
/// Do not expand the root node after reloading
#define wxVDTC_NO_EXPAND    4
/// Do not call sort. Simply display files as they are located
#define wxVDTC_NO_SORT      8
/// Do not add files, only directories
#define wxVDTC_NO_FILES    16

/** \class VdtcTreeItemBase
	This class is used to link the information of a file/root or folder node to a wxTreeItem
	in the wxTreeCtrl.

	To use more information in this class, inherit this with your own copy, and in the handler
	wxVirtualDirTreeCtrl::OnCreateTreeItem, return your own class. This allows you to store more
	information in this class.

	For example; to return a different caption instead of the standard filename / directory, inherit this, and
	redefine the VdtcTreeItemBase::GetCaption() member. The same goes for the VdtcTreeItemBase::GetIconId()
	and VdtcTreeItemBase::GetSelectedIconId() functions if you redefined the bitmaps in the imagelist.

*/
class VdtcTreeItemBase : public wxTreeItemData
{
protected:
	wxString _name;
	int _type;

public:
	/** Default constructor. Pass the parent of this node as a VdtcTreeItemBase object, the type
	    of this object (VDTC_TI_ROOT,VDTC_TI_DIR, VDTC_TI_FILE or added types but leave these
		three in place), and the name of the object (i.e. filename or dir name). When deriving this
		class make sure you call the constructor propertly. The types are of essence for some
		internal functions. For example:

		\code

		class MyOwnData : public VdtcTreeItemBase
		{
			MyOwnData(int type, const wxString &name)
				: VdtcTreeItemBase(type, name)
			{
				// do initialisation
			}
		}
		\endcode

	*/
	VdtcTreeItemBase(int type, const wxString &name)
		: _type(type)
		, _name(name)
	{
	};

	/** Default destructor */
	~VdtcTreeItemBase()
	{
		// NOTE: do not delete the tree item
		// because the tree item deletes this item data
	};

	/** Virtual function to report the caption back to the wxTreeCtrl to be added. If the caption should be
	    something else then the default name it gets from the file (or the root path when this node points
		to a root item, inherit this class and redefine GetCaption
	*/
	virtual const wxString &GetCaption() const {
		return _name;
	};

	/** Virtual function to return the icon ID this node should get. Per default it gets the ID of the
	    default image list. If you assigned more bitmaps (or different bitmaps) to the image list, return
		the proper indices based upon the class it refers to. The ID's returned are:

		- VDTC_ICON_ROOT: For root
		- VDTC_ICON_DIR: For a directory
		- VDTC_ICON_FILE: For a file
	*/
	virtual int GetIconId() const {
		switch(_type)
		{
			case VDTC_TI_ROOT:
				return VDTC_ICON_ROOT;
			case VDTC_TI_DIR:
				return VDTC_ICON_DIR;
			case VDTC_TI_FILE:
				return VDTC_ICON_FILE;
		}
		return -1;
	};

	/** Virtual function to return the selected icon ID this node should get. Per default there is no icon
	    associated with a selection. If you would like a selection, inherit this class and redefine this function
		to return a proper id.
	*/
	virtual int GetSelectedIconId() const {
		return -1;
	};

	/** Gets this name. The name of the root is the base path of the whole directory, the
	   name of a file node is the filename, and from a dir node the directory name.
	   \sa IsDir, IsFile, IsRoot */
	const wxString &GetName() {
		return _name;
	};

	/** Returns true if this is of type VDTC_TI_DIR */
	bool IsDir() const {
		return _type == VDTC_TI_DIR;
	};

	/** Returns true if this is of type VDTC_TI_ROOT */
	bool IsRoot() const {
		return _type == VDTC_TI_ROOT;
	};

	/** Returns true if this is of type VDTC_TI_FILE */
	bool IsFile() const {
		return _type == VDTC_TI_FILE;
	};

};

// the formal definition of the array of pointers for tree item base
WX_DEFINE_ARRAY(VdtcTreeItemBase *, VdtcTreeItemBaseArray);

/**
	\class wxVirtualDirTreeCtrl
	A class which allows the user to browse through a 'virtual' treectrl, fully customizable when it's derived.

	The wxVirtualDirTreeCtrl can be used for displaying the directory structure in a tree ctrl just like the
	wxDirCtrl, but has much more advantages and flexibility for the developer. A list of some of the functionality
	embedded:

	See the main page to read more about this control.
*/

class wxVirtualDirTreeCtrl : public wxTreeCtrl
{
private:
	/** File extensions list */
	wxArrayString _extensions;
	/** Icons image list */
	wxImageList *_iconList;
	/** Extra flags */
	int _flags;

	/** Scans from given dir, for 'level' depth and with present extensions. This will
	    reload the directory on that level. If there are tree items associated with the 'reloaded'
		items they will be deleted first. */
	int ScanFromDir(VdtcTreeItemBase *item, const wxFileName &path, int level);

	/** Get files for current dir */
	void GetFiles(VdtcTreeItemBase *parent, VdtcTreeItemBaseArray &items, const wxFileName &path);

	/** get dirs in current dir */
	void GetDirectories(VdtcTreeItemBase *parent, VdtcTreeItemBaseArray &items, const wxFileName &path);

	void AppendPathRecursively(VdtcTreeItemBase *b, wxFileName &dir, bool useRoot = true);

	/** Adds items on this level to the parent dir associated wxTreeCtrl item */
	void AddItemsToTreeCtrl(VdtcTreeItemBase *item, VdtcTreeItemBaseArray &items);

	/** Sorts items with qsort algorithm */
	void SortItems(VdtcTreeItemBaseArray &items, int left, int right);

	/** Built in swap algoritm */
	void SwapItem(VdtcTreeItemBaseArray &items, int a, int b);

	// -- event handlers --

	void OnExpanding(wxTreeEvent &event);

protected:
	/** This method can be used in the method OnAssignIcons. It returns a pointer to a newly created bitmap
	    holding the default icon image for a root node. NOTE: When this bitmap is assigned to the icon list,
		don't forget to delete it! */
	wxBitmap *CreateRootBitmap();

	/** This method can be used in the method OnAssignIcons. It returns a pointer to a newly created bitmap
	    holding the default icon image for a folder node. NOTE: When this bitmap is assigned to the icon list,
		don't forget to delete it! */
	wxBitmap *CreateFolderBitmap();

	/** This method can be used in the method OnAssignIcons. It returns a pointer to a newly created bitmap
	    holding the default icon image for a file node. NOTE: When this bitmap is assigned to the icon list,
		don't forget to delete it! */
	wxBitmap *CreateFileBitmap();

	/** Inherited virtual function for SortChildren */
	int OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2);

public:
    /** Default constructor of this control. It is similar to the wxTreeCtrl */
    wxVirtualDirTreeCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT,
                  const wxValidator& validator = wxDefaultValidator,
                  const wxString& name = _T("wxVirtualDirTreeCtrl"));
    virtual ~wxVirtualDirTreeCtrl();

	/** Returns the extra flags currently set set for wxVirtualDirTreeCtrl.
		\sa SetRootPath */
	int GetExtraFlags() const {
		return _flags;
	};

	/** Sets the root path of the wxVirtualDirTreeCtrl. This will reset the view and restart the
	    process of notification, and all events that need to be called. Returns false when this
		root path does not lead to any valid filesystem or directory. Set extra flags for
		wxVirtualDirTreeCtrl when needed. The following flags are possible
		- wxVDTC_DEFAULT
		- wxVDTC_RELOAD_ALL
		- wxVDTC_NO_EXPAND
		- wxVDTC_SHOW_BUSYDLG
		- wxVDTC_NO_SORT
		- wxVDTC_NO_FILES */
	bool SetRootPath(const wxString &root, int flags = wxVDTC_DEFAULT);

	/** Resets the extensions to "*.*" as default (or "*" under linux). The extension will be used
		upon next reload of the directory tree. In between extensions can be changed but will have
		no effect upon the directory scanning mechanism until the next SetRootPath */
	void ResetExtensions() {
		_extensions.Clear();
		_extensions.Add(VDTC_DIR_FILESPEC);
	};

	/** Returns the relative path of the item with the given wxTreeItemId. This doesn't include the
	    root node. This can be used in ExpandToPath(). A check is made if the object belonging to this
		wxTreeItemId is of the type directory or file. In both cases the relative path (plus filename if
		it is a file) is returned. When the object pointed out by wxTreeItemId is of the type root, an
		empty wxFileName is returned. Be warned, these functions can take some time because the whole tree
		structure is iterated from the top node down to the root */
	wxFileName GetRelativePath(const wxTreeItemId &id);

	/** Return full path of given node. This can also include the name of the file. This will be returned as
	    a wxFileName (with path and possible name) to be used further. Be warned, these functions can take some time because the whole tree
		structure is iterated from the top node down to the root. */
	wxFileName GetFullPath(const wxTreeItemId &id);

	/** Returns TRUE when the item pointed out by wxTreeItemId is a root node internally, this means the
		assigned VdtcTreeItemBase class is of type VDTC_TI_ROOT.
		\sa VdtcTreeItemBase::IsRoot */
	bool IsRootNode(const wxTreeItemId &id);

	/** Returns TRUE when the item pointed out by wxTreeItemId is a directory node internally, this means the
		assigned VdtcTreeItemBase class is of type VDTC_TI_DIR. For these types of nodes you can use i.e.
		GetRelativePath() or GetFullPath() to retrieve the directory.
		\sa VdtcTreeItemBase::IsDir, GetRelativePath, GetFullPath */
	bool IsDirNode(const wxTreeItemId &id);

	/** Returns TRUE when the item pointed out by wxTreeItemId is a file node internally, this means the
		assigned VdtcTreeItemBase class is of type VDTC_TI_DIR. For these types of nodes you can use i.e.
		GetRelativePath() or GetFullPath() to retrieve the directory plus filename.
		\sa VdtcTreeItemBase::IsFile, GetRelativePath, GetFullPath */
	bool IsFileNode(const wxTreeItemId &id);

	/** Expands from root, to the given path. Every path in the wxFileName is looked up and expanded. When
	    a path section is not found, this method aborts and returns a wxTreeItemId with value 0. When it succeeds
		the wxTreeItemId is returned of the last tree item that was expanded.

		<b>A special note:</b> Don't include the root path of the current tree in the filename. This part of the path
		is not used to scan. Use GetRelativePath to get the path up until a specific node. This method is useful
		to expand earlier paths after restoring the window state.
		\sa GetRelativePath */
	wxTreeItemId ExpandToPath(const wxFileName &path);

	/** Adds a file item. Be aware that this call does not add it to the wxTreeCtrl. This only creates an instance
	    of a VtdcTreeItemBase file node. The handler OnCreateTreeItem is called to allow the proper initialisation
		of every newly created instance of the (inherited) VdtcTreeItemBase class. */
	VdtcTreeItemBase *AddFileItem(const wxString &name);

	/** Adds a directory item. Be aware that this call does not add it to the wxTreeCtrl. This only creates an instance
	    of a VtdcTreeItemBase directory node. The handler OnCreateTreeItem is called to allow the proper initialisation
		of every newly created instance of the (inherited) VdtcTreeItemBase class. */
	VdtcTreeItemBase *AddDirItem(const wxString &name);

	/** Returns parent of the passed VdtcItemBase object. It will fetch the wxTreeItemId of this parent,
	    and return the VdtcTreeItemBase parent associated with it. If the associated item is nil, there is no
		parent, this is most likely the root else an assertion failure occurs */
	VdtcTreeItemBase *GetParent(VdtcTreeItemBase *item) const {
		wxCHECK(item, 0);

		wxTreeItemId p = GetItemParent(item->GetId());
		if (! p.IsOk()) return NULL;
		return (VdtcTreeItemBase *)GetItemData(p);
	};

	// --- handlers ---

	/** This handler is called when the SetRootPath function is called. This call causes a re-initialisation of
	    the wxVirtualDirTreeCtrl. It can be useful to initialise the class that derived this class as well. The
		root is passed as parameter to inform which root is going to be set. NOTE: When this method is called,
		the following criteria is true:
		- The tree is completely empty
		- The path parameter is valid. When it's not valid this call is not made */
	void OnSetRootPath(const wxString &root);

	/** This virtual handler is used to allow the developer to assign custom icons to the
	    image list. Override this method to assign your own icons to the wxTreeCtrl. The default method
		will assign a root bitmap, a folder bitmap and a file bitmap. The icons assigned can be
		refered to in the OnFileName and OnDirectory handlers, to specify what icon should be used.

		The default images can be used by these constants:
		- VDTC_ICON_ROOT
		- VDTC_ICON_DIR
		- VDTC_ICON_FILE

		The icon list contains icons which are 16x16 in size.
	*/
	virtual void OnAssignIcons(wxImageList &icons);

	/** This handler provides must provide a VdtcTreeItemBase* instance to be used upon creation. If you
	    override this handler, you can return your own class, which might embed more information.

		The type tells you what kind of element is created, a root item (VDTC_TI_ROOT), a dir item (VDTC_TI_DIR)
		or a file item (VDTC_TI_FILE). Dependent upon the type you can return a different class based upon
		VdtcTreeItemBase.

		For example, this is the standard implementation:

		\code
			return new VdtcTreeItemBase(type, name);
		\endcode

		You could derive the wxVirtualDirTreeCtrl and VdtcTreeItemBase object and make this:

		\code

			switch(type)
			{
				case VDTC_TI_ROOT:
					return new MyRootItem(type, name);	// MyRootItem = class derived VdtcTreeItemBase
				case VDTC_TI_DIR:
					return new MyDirItem(type, name);	// MyDirItem = class derived VdtcTreeItemBase
				case VDTC_TI_FILE:
					return new MyFileItem(type, name);	// MyFileItem = class derived VdtcTreeItemBase
			}

			return 0;

		\endcode

		\sa VdtcTreeItemBase
	*/
	virtual VdtcTreeItemBase *OnCreateTreeItem(int type, const wxString &name);

	/** This handler is called before the VdtcTreeItemBase item is added to the root. This will be the first item to be added
	    before all else is added.

		If for some reason this root is denied to be added, return FALSE. If it may be added, return TRUE. If false is returned
		then no other items are created, and the SetRootPath() function returns with FALSE notifying the caller that something
		went wrong.
		\sa VdtcTreeItemBase
	*/
	virtual bool OnAddRoot(VdtcTreeItemBase &item, const wxFileName &name);

	/** This handler is called before a file VdtcTreeItemBase item is added to the tree. For every file added in the tree this
		handler is called. It allows you to change the name or caption of the item, and also gain more information
		or change the contents of the associated VdtcTreeItemBase class. The wxFileName contains the full path
		of the item to be added, so it can be easily inspected, and tested for specific extensions, parts of the
		name and what else.

		If for some reason this item is denied to be added, return FALSE. If it may be added, return TRUE.
	*/
	virtual bool OnAddFile(VdtcTreeItemBase &item, const wxFileName &name);

	/** This handler is called before a directory item is added to the tree. For every dir added in the tree this
		handler is called. It allows you to change the name of the item or the caption, and also gain more information of the
		directory, and check if specific files are present. The wxFileName contains the full path of the directory
		to be added, so it can be easily inspected, and tested for specific criteria, parts of the name and what else.

		If for some reason this item is denied to be added, return FALSE. If it may be added, return TRUE.
	*/
	virtual bool OnAddDirectory(VdtcTreeItemBase &item, const wxFileName &name);

	/** This handler is called before the directory specified by 'path' is scanned. You can veto this scan by
	    returning 'false'. This way the scanning of all files and directories in this path is skipped. After this
		handler, subsequent calls to OnAddFile, OnAddDirectory will be made for every file and directory
		encountered in this level to be scanned. NOTE: When this scan is veto'd there will be no call
		to OnDirectoryScanEnd because there was no scan. Also OnAddedItems is not called */

	virtual bool OnDirectoryScanBegin(const wxFileName &path);

	/** This handler is called when all files and all directories are scanned in the current dir and iterated in
	    the array passed to this method. Before the sorting algorithm is initiated, the developer is allowed to
	    take one last look at all the files, maybe delete some or act upon other criteria. The items parameter
	    contains the pointer array of all the items that are in the list, and the path parameter contains the
	    current path investigated. NOTE: If you want to delete an item from the array, delete it with delete
	    operator and remove the pointer from the list.  */

	virtual void OnDirectoryScanEnd(VdtcTreeItemBaseArray &items, const wxFileName &path);

	/** This handler is called the very moment after all items are added to the tree control. The parent parameter
	    contains the tree node to which the items are added. They are already sorted and ready. E.g. when you don't
		want to use the internal sort algoritm this is a nice place to call SortChildren on the parent node */

	virtual void OnAddedItems(const wxTreeItemId &parent);

	/** This handler is called during the sorting of the tree control items. Return < 0 when a < b, > 0 when a > b
	    and 0 when a == b.
	*/
	virtual int OnCompareItems(const VdtcTreeItemBase *a, const VdtcTreeItemBase *b);


private:
    // WDR: member variable declarations for wxVirtualDirTreeCtrl

private:
    // WDR: handler declarations for wxVirtualDirTreeCtrl

private:
    DECLARE_EVENT_TABLE()
};




#endif
