#include "AtlasWindowCommand.h"

#include "wx/arrstr.h"
#include "AtlasObject/AtlasObject.h"
//#include "RefcntPtr.h"

class DraggableListCtrl;

class DragCommand : public AtlasWindowCommand
{
	DECLARE_CLASS(DragCommand)

public:
	DragCommand(DraggableListCtrl* ctrl, long src, long tgt);
	bool Do();
	bool Undo();

private:
	bool Merge(AtlasWindowCommand* command);

	DraggableListCtrl* m_Ctrl;
	long m_Src, m_Tgt;

	std::vector<AtObj> m_OldData;
};


class DeleteCommand : public wxCommand
{
public:
	DeleteCommand(DraggableListCtrl* ctrl, long itemID);
	bool Do();
	bool Undo();

private:
	DraggableListCtrl* m_Ctrl;
	//wxArrayString m_Texts;
	long m_ItemID;
	//RefcntPtr<AtObj> m_ItemData;
	std::vector<AtObj> m_OldData;
};
