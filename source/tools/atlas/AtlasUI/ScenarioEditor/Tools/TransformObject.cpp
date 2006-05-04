#include "stdafx.h"

#include "Common/Tools.h"
#include "Common/Brushes.h"
#include "Common/MiscState.h"
#include "GameInterface/Messages.h"

using AtlasMessage::Position;

class TransformObject : public StateDrivenTool<TransformObject>
{
	DECLARE_DYNAMIC_CLASS(TransformObject);

	int m_dx, m_dy;

public:
	TransformObject()
	{
		SetState(&Waiting);
	}

	void OnDisable()
	{
		g_SelectedObjects.clear();
		g_SelectedObjects.NotifyObservers();
		POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));
	}


	// TODO: keys to rotate/move object?

	struct sWaiting : public State
	{
		bool OnMouse(TransformObject* obj, wxMouseEvent& evt)
		{
			if (evt.LeftDown())
			{
				// TODO: multiple selection
				AtlasMessage::qPickObject qry(Position(evt.GetPosition()));
				qry.Post();
				g_SelectedObjects.clear();
				if (AtlasMessage::ObjectIDIsValid(qry.id))
				{
					g_SelectedObjects.push_back(qry.id);
					obj->m_dx = qry.offsetx;
					obj->m_dy = qry.offsety;
					SET_STATE(Dragging);
				}
				g_SelectedObjects.NotifyObservers();
				POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));
				ScenarioEditor::GetCommandProc().FinaliseLastCommand();
				return true;
			}
			else if (evt.Dragging() && evt.RightIsDown() || evt.RightDown())
			{
				Position pos (evt.GetPosition());
				for (size_t i = 0; i < g_SelectedObjects.size(); ++i)
					POST_COMMAND(RotateObject, (g_SelectedObjects[i], true, pos, 0.f));
				return true;
			}
			else
				return false;
		}

		bool OnKey(TransformObject* obj, wxKeyEvent& evt, KeyEventType type)
		{
			if (type == KEY_CHAR && evt.GetKeyCode() == WXK_DELETE)
			{
				for (size_t i = 0; i < g_SelectedObjects.size(); ++i)
					POST_COMMAND(DeleteObject, (g_SelectedObjects[i]));
				g_SelectedObjects.clear();
				g_SelectedObjects.NotifyObservers();
				POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));
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
				for (size_t i = 0; i < g_SelectedObjects.size(); ++i)
					POST_COMMAND(MoveObject, (g_SelectedObjects[i], pos));
				return true;
			}
			else
				return false;
		}
	}
	Dragging;
};

IMPLEMENT_DYNAMIC_CLASS(TransformObject, StateDrivenTool<TransformObject>);
