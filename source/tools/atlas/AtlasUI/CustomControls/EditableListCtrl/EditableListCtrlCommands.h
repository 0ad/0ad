#include "AtlasWindowCommand.h"

#include "AtlasObject/AtlasObject.h"

class EditableListCtrl;

class EditCommand_Dialog : public AtlasWindowCommand
{
	DECLARE_CLASS(EditCommand_Dialog);

public:
	EditCommand_Dialog(EditableListCtrl* ctrl, long row, int col, AtObj& newData);

	bool Do();
	bool Undo();

private:
	EditableListCtrl* m_Ctrl;
	long m_Row;
	int m_Col;
	AtObj m_NewData;
	std::vector<AtObj> m_OldData;

};


class EditCommand_Text : public AtlasWindowCommand
{
	DECLARE_CLASS(EditCommand_Text);

public:
	EditCommand_Text(EditableListCtrl* ctrl, long row, int col, wxString newText);

	bool Do();
	bool Undo();

private:
	EditableListCtrl* m_Ctrl;
	long m_Row;
	int m_Col;
	wxString m_NewText;
	std::vector<AtObj> m_OldData;
};


class PasteCommand : public AtlasWindowCommand
{
	DECLARE_CLASS(PasteCommand);

public:
	PasteCommand(EditableListCtrl* ctrl, long row, AtObj& newData);
	bool Do();
	bool Undo();

private:
	EditableListCtrl* m_Ctrl;
	long m_Row;
	AtObj m_NewData;
	std::vector<AtObj> m_OldData;
};

