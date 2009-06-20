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

#ifndef INCLUDED_FIELDEDITCTRL
#define INCLUDED_FIELDEDITCTRL

class EditableListCtrl;
class AtlasDialog;

class FieldEditCtrl
{
	friend class EditableListCtrl;

public:
	virtual ~FieldEditCtrl() {};

protected:
	virtual void StartEdit(wxWindow* parent, wxRect rect, long row, int col)=0;
};

//////////////////////////////////////////////////////////////////////////

class FieldEditCtrl_Text : public FieldEditCtrl
{
protected:
	void StartEdit(wxWindow* parent, wxRect rect, long row, int col);
};

//////////////////////////////////////////////////////////////////////////

class FieldEditCtrl_Colour : public FieldEditCtrl
{
protected:
	void StartEdit(wxWindow* parent, wxRect rect, long row, int col);
};

//////////////////////////////////////////////////////////////////////////

class FieldEditCtrl_List : public FieldEditCtrl
{
public:
	// listType must remain valid at least until StartEdit has been called
	FieldEditCtrl_List(const char* listType);

protected:
	void StartEdit(wxWindow* parent, wxRect rect, long row, int col);

private:
	const char* m_ListType;
};

//////////////////////////////////////////////////////////////////////////

class FieldEditCtrl_Dialog : public FieldEditCtrl
{
public:
	FieldEditCtrl_Dialog(AtlasDialog* (*dialogCtor)(wxWindow*));

protected:
	void StartEdit(wxWindow* parent, wxRect rect, long row, int col);

private:
	AtlasDialog* (*m_DialogCtor)(wxWindow*);
};

//////////////////////////////////////////////////////////////////////////


class FieldEditCtrl_File : public FieldEditCtrl
{
public:
	// rootDir is relative to mods/*/, and must end with a /
	FieldEditCtrl_File(const wxString& rootDir, const wxString& fileMask);

protected:
	void StartEdit(wxWindow* parent, wxRect rect, long row, int col);

private:
	wxString m_RootDir; // relative to mods/*/
	wxString m_FileMask;
	wxString m_RememberedDir;
};

#endif // INCLUDED_FIELDEDITCTRL
