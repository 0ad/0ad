#include "General/AtlasWindowCommand.h"

#include "AtlasObject/AtlasObject.h"

#include <vector>

class DraggableListCtrl;

class DragCommand : public AtlasWindowCommand
{
	DECLARE_CLASS(DragCommand);

public:
	DragCommand(DraggableListCtrl* ctrl, long src, long tgt);
	bool Do();
	bool Undo();

private:
	bool Merge(AtlasWindowCommand* previousCommand);

	DraggableListCtrl* m_Ctrl;
	long m_Src, m_Tgt;

	std::vector<AtObj> m_OldData;
};


class DeleteCommand : public AtlasWindowCommand
{
	DECLARE_CLASS(DeleteCommand);

public:
	DeleteCommand(DraggableListCtrl* ctrl, long itemID);
	bool Do();
	bool Undo();

private:
	DraggableListCtrl* m_Ctrl;
	long m_ItemID;
	std::vector<AtObj> m_OldData;
};
