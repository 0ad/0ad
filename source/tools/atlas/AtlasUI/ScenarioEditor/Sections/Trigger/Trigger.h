/* Desc: interface for creation and editing of triggers.  Interprets
   XML specifications then uses this to construct layout.
*/

#include "../Common/Sidebar.h"
#include "GameInterface/Messages.h"

class TriggerItemData;
class TriggerTreeCtrl;
class TriggerBottomBar;
class TriggerPage;
class wxTreeItemId;
class wxTreeItemData;
class wxTreeEvent;
class wxListEvent;
class wxNotebook;
class wxNotebookEvent;
class wxTreeEvent;
class wxListEvent;

class TriggerSidebar : public Sidebar
{
public:
	enum { ID_CondList, ID_EffectList };

	TriggerSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer);

	void onTreeDrag(wxTreeEvent& evt);
	void onTreeNameChange(wxTreeEvent& evt);
	//void onTreeDelete(wxTreeEvent& evt);
	void onTreeSelChange(wxTreeEvent& evt);
	void onTreeCollapse(wxTreeEvent& evt);
	void onListSelect(wxListEvent& evt);
	void onEffectSelect(wxListEvent& evt);
	void onCondSelect(wxListEvent& evt);
	void onPageChange(wxNotebookEvent& evt);

	bool IsGroupSelected();
	TriggerItemData* ToDerived(wxTreeItemData* data);
	TriggerItemData* GetSelectedItemData();
	
	//Finds condition number (index+1) of m_SelectedCond [needed because of logic blocks]
	int GetConditionCount(int limit);
	int GetLogicBlockCount(int limit);
	void UpdateLists();
	void UpdateEngineData();

	//AtlasMessage::sTriggerList m_TriggerList;

	TriggerBottomBar* m_TriggerBottom;
	TriggerTreeCtrl* m_TriggerTree;
	TriggerPage* m_ConditionPage, *m_EffectPage;
	wxNotebook* m_Notebook;

	size_t m_TriggerCount, m_GroupCount;
	long m_SelectedCond, m_SelectedEffect, m_SelectedCondIndex, m_SelectedEffectIndex;
	wxString m_LogicBlockString, m_LogicBlockEndString;

protected:
	virtual void OnFirstDisplay();

private:
	struct copyIfRootChild
	{
		copyIfRootChild(std::vector<AtlasMessage::sTriggerGroup>& groupList) : m_groupList(groupList) {}
		void operator() ( const AtlasMessage::sTriggerGroup& group )
		{ 
			if ( *group.parentName == std::wstring(L"Triggers") )
				m_groupList.push_back(group);
		}
	private:
		std::vector<AtlasMessage::sTriggerGroup>& m_groupList;
	};
	
	void AddGroupTree(const AtlasMessage::sTriggerGroup& group, wxTreeItemId parent);
	
	void CreateGroup(std::vector<AtlasMessage::sTriggerGroup>& groupList, 
					AtlasMessage::sTriggerGroup& parent, wxTreeItemId index);

	AtlasMessage::sTrigger CreateTrigger(TriggerItemData* data);

	std::vector<AtlasMessage::sTriggerGroup> m_TriggerGroups;

	DECLARE_EVENT_TABLE();
};


