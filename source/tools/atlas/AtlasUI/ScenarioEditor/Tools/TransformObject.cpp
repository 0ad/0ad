#include "stdafx.h"

#include "Common/Tools.h"
#include "Common/Brushes.h"
#include "Common/MiscState.h"
#include "GameInterface/Messages.h"

using AtlasMessage::Position;

class TransformObject : public StateDrivenTool<TransformObject>
{
	DECLARE_DYNAMIC_CLASS(TransformObject);

	std::vector<AtlasMessage::ObjectID> m_Selection;
	int m_dx, m_dy;

public:
	TransformObject()
	{
		SetState(&Waiting);
	}

	void OnDisable()
	{
		m_Selection.clear();
		POST_MESSAGE(SetSelectionPreview, (m_Selection));
	}


	// TODO: keys to rotate/move object?

	struct sWaiting : public State
	{
		bool OnMouse(TransformObject* obj, wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				// TODO: multiple selection
				AtlasMessage::qSelectObject qry(Position(evt.GetPosition()));
				qry.Post();
				obj->m_Selection.clear();
				if (AtlasMessage::ObjectIDIsValid(qry.id))
				{
					obj->m_Selection.push_back(qry.id);
					obj->m_dx = qry.offsetx;
					obj->m_dy = qry.offsety;
					SET_STATE(Dragging);
				}
				POST_MESSAGE(SetSelectionPreview, (obj->m_Selection));
				ScenarioEditor::GetCommandProc().FinaliseLastCommand();
				return true;
			}
			else if (evt.Dragging() && evt.RightIsDown() || evt.RightDown())
			{
				Position pos (evt.GetPosition());
				for (size_t i = 0; i < obj->m_Selection.size(); ++i)
					POST_COMMAND(RotateObject, (obj->m_Selection[i], true, pos, 0.f));
				return true;
			}
			else
				return false;
		}

		bool OnKey(TransformObject* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_CHAR && evt.GetKeyCode() == WXK_DELETE)
			{
				for (size_t i = 0; i < obj->m_Selection.size(); ++i)
					POST_COMMAND(DeleteObject, (obj->m_Selection[i]));
				obj->m_Selection.clear();
				POST_MESSAGE(SetSelectionPreview, (obj->m_Selection));
				return true;
			}
			else
				return false;
		}
	}
	Waiting;

	struct sDragging : public State
	{
		bool OnMouse(TransformObject* obj, wxMouseEvent& evt)
		{
			if (evt.LeftUp())
			{
				SET_STATE(Waiting);
				return true;
			}
			else if (evt.Dragging())
			{
				Position pos (evt.GetPosition() + wxPoint(obj->m_dx, obj->m_dy));
				for (size_t i = 0; i < obj->m_Selection.size(); ++i)
					POST_COMMAND(MoveObject, (obj->m_Selection[i], pos));
				return true;
			}
			else
				return false;
		}
	}
	Dragging;
};

IMPLEMENT_DYNAMIC_CLASS(TransformObject, StateDrivenTool<TransformObject>);
