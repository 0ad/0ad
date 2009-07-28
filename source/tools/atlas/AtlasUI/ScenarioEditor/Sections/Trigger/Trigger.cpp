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

#include "precompiled.h"

#include "Trigger.h"
#include "GameInterface/Messages.h"
#include "CustomControls/Buttons/ActionButton.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "ScenarioEditor/Tools/Common/MiscState.h"

#include "wx/treectrl.h"
#include "wx/listctrl.h"
#include "wx/notebook.h"
#include <sstream>
#include <list>

using namespace AtlasMessage;

BEGIN_EVENT_TABLE(TriggerSidebar, Sidebar)
EVT_TREE_BEGIN_DRAG(wxID_ANY, TriggerSidebar::onTreeDrag)
EVT_TREE_END_LABEL_EDIT(wxID_ANY, TriggerSidebar::onTreeNameChange)
EVT_TREE_SEL_CHANGED(wxID_ANY, TriggerSidebar::onTreeSelChange)
EVT_LIST_ITEM_SELECTED(TriggerSidebar::ID_CondList, TriggerSidebar::onCondSelect)
EVT_LIST_ITEM_SELECTED(TriggerSidebar::ID_EffectList, TriggerSidebar::onEffectSelect)
EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, TriggerSidebar::onPageChange)
END_EVENT_TABLE()


class TriggerTreeCtrl : public wxTreeCtrl
{
public:
	TriggerTreeCtrl(TriggerSidebar* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, 
						const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS )
						: wxTreeCtrl(parent, id, pos, size, style), m_Sidebar(parent) { }
	
	void onClick(wxMouseEvent& evt);

private:
	TriggerSidebar* m_Sidebar;
	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(TriggerTreeCtrl, wxTreeCtrl)
EVT_LEFT_DOWN(TriggerTreeCtrl::onClick)
END_EVENT_TABLE()


struct LogicBlockHelper
{
	LogicBlockHelper() { index = -1; end = false; }
	LogicBlockHelper(int _index, bool _end) : index(_index), end(_end) {}
	int index;
	bool end;

	bool operator< ( const LogicBlockHelper& rhs ) const
	{
		return index < rhs.index;
	}
	bool operator== ( const LogicBlockHelper& rhs ) const
	{
		return index == rhs.index;
	}
};
class TriggerSpecText : public wxTextCtrl
{
	typedef void (*callback)(void* data, std::wstring input, int parameter);

public:
	TriggerSpecText(wxWindow* parent, std::wstring label, const wxPoint& pos, const wxSize& size, 
						int parameter, std::wstring dataType, callback func, void* data)
		: wxTextCtrl(parent, wxID_ANY, wxString( label.c_str() ), pos, size, wxTE_PROCESS_ENTER), 
			m_DataType(dataType), m_Parameter(parameter), m_Data(data), m_Callback(func)
	{
	}
	void onTextEnter(wxCommandEvent& WXUNUSED(evt));

	//Disallow invalid input
	bool VerifyInput(std::wstring& input)
	{
		std::wstringstream stream(input);
		if ( m_DataType == L"int" )
		{
			int test;
			stream >> test;
			return !stream.fail();
		}
		else if ( m_DataType == L"real" )
		{
			float test;
			stream >> test;
			return !stream.fail();
		}
		else if ( m_DataType == L"bool" )
		{
			bool test;
			stream >> test;
			return !stream.fail();
		}
		else if ( m_DataType == L"string" )
		{
			//Make strings appear as strings to javascript
			std::wstring quote(L"\"");
			input.insert(0, quote);
			input.append(quote);
			return true;
		}
		else
		{
			wxFAIL_MSG(L"Invalid input type for trigger specification");
			return false;
		}
	}
private:
	void* m_Data;
	int m_Parameter;
	std::wstring m_DataType;
	callback m_Callback;

	DECLARE_EVENT_TABLE();
	
};
BEGIN_EVENT_TABLE(TriggerSpecText, wxTextCtrl)
EVT_TEXT_ENTER(wxID_ANY, TriggerSpecText::onTextEnter)
END_EVENT_TABLE()

class TriggerSpecChoice : public wxChoice
{
	typedef void (*callback)(void* data, std::wstring input, int parameter);

public:
	TriggerSpecChoice(TriggerBottomBar* parent, std::wstring label, const wxPoint& pos, 
		const wxSize& size, const wxArrayString& strings, int parameter, callback func, void* data);

	void onChoice(wxCommandEvent& evt);
	
private:
	TriggerBottomBar* m_Parent;
	callback m_Callback;
	int m_Parameter;
	void* m_Data;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(TriggerSpecChoice, wxChoice)
EVT_CHOICE(wxID_ANY, TriggerSpecChoice::onChoice)
END_EVENT_TABLE()

class TriggerBottomBar;

class TriggerEntitySelector : public wxPanel
{
	typedef void (*callback)(void* data, std::wstring input, int parameter);
	enum { ID_SELECTION, ID_VIEW };
public:

	TriggerEntitySelector(TriggerBottomBar* parent, std::wstring label, const wxPoint& pos, 
		const wxSize& size, int parameter, callback func, void* data);
	
	void onSelectionClick(wxCommandEvent& WXUNUSED(evt))
	{
		std::wstring code(L"[");
		std::wstringstream stream;
		for ( size_t i = 0; i < g_SelectedObjects.size(); ++i )
		{
			stream << g_SelectedObjects[i];
			if ( i != g_SelectedObjects.size()-1 )
				stream << L", ";
		}

		code.append(stream.str());
		code.append(L"]");
		(*m_Callback)(m_Data, code, m_Parameter);
		POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));
	}
	void onViewClick(wxCommandEvent& WXUNUSED(evt));

private:
	int m_Parameter;
	callback m_Callback;
	TriggerBottomBar* m_Parent;
	void* m_Data;

	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(TriggerEntitySelector, wxPanel)
EVT_BUTTON(TriggerEntitySelector::ID_SELECTION, TriggerEntitySelector::onSelectionClick)
EVT_BUTTON(TriggerEntitySelector::ID_VIEW, TriggerEntitySelector::onViewClick)
END_EVENT_TABLE()

class TriggerPointPlacer : public wxPanel
{
	enum { ID_Set, ID_View };
	typedef void (*callback)(void* data, std::wstring input, int parameter);
public:

	TriggerPointPlacer(wxWindow* parent, const wxPoint& pos, 
		const wxSize& size, int parameter, callback func, void* data) : m_Callback(func),
		wxPanel(parent, wxID_ANY, pos), m_Parameter(parameter), m_Data(data)
	{
		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(mainSizer);
		mainSizer->Add( new wxButton(this, ID_Set, L"Set point", pos, size) );
		mainSizer->Add( new wxButton(this, ID_View, L"View", pos, size) );
	}

	
	void onSet(wxCommandEvent& WXUNUSED(evt))
	{
		qGetWorldPosition query( wxGetMousePosition().x, wxGetMousePosition().y );
		query.Post();
		Position pos = query.position;
		wxString wxForm = wxString::Format(L"Vector(%f, %f, %f)", pos.type0.x, pos.type0.y, pos.type0.z);
											
		std::wstring convert = (std::wstring)wxForm;
		(*m_Callback)(m_Data, convert, m_Parameter);
		POST_MESSAGE(TriggerToggleSelector, (true, pos));
	}

	void onView(wxCommandEvent& WXUNUSED(evt))
	{
		//POST_MESSAGE( TriggerToggleSelector, (pos) );
	}

private:
	int m_Parameter;
	void* m_Data;
	callback m_Callback;
	bool m_MouseCapture;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(TriggerPointPlacer, wxPanel)
EVT_BUTTON(TriggerPointPlacer::ID_Set, TriggerPointPlacer::onSet)
EVT_BUTTON(TriggerPointPlacer::ID_View, TriggerPointPlacer::onView)
END_EVENT_TABLE()


class TriggerListCtrl : public wxListCtrl
{
public:
	TriggerListCtrl(wxWindow* parent, TriggerSidebar* sidebar, bool condition, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
						const wxSize& size = wxDefaultSize, long style = wxLC_ICON)
		: wxListCtrl(parent, id, pos, size, style), m_Sidebar(sidebar), m_Condition(condition)
	{
	}
	void onClick(wxMouseEvent& evt);

	TriggerSidebar* m_Sidebar;
	bool m_Condition;
private:
	DECLARE_EVENT_TABLE();
};
BEGIN_EVENT_TABLE(TriggerListCtrl, wxListCtrl)
EVT_LEFT_DOWN(TriggerListCtrl::onClick)
END_EVENT_TABLE()


class TriggerPage : public wxPanel
{
public:
	TriggerPage(wxWindow* parent, TriggerSidebar* sidebar, long ID, wxString title, bool condition) 
		: wxPanel(parent), m_Sidebar(sidebar), m_Condition(condition)
	
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

		m_List = new TriggerListCtrl(this, sidebar, condition, ID, wxDefaultPosition,
					wxSize(132, 210), wxLC_REPORT | wxLC_SINGLE_SEL);
		m_List->InsertColumn(0, title, wxLIST_FORMAT_LEFT, 100);

		sizer->Add(m_List);
		SetSizer(sizer);
	}

	wxListCtrl* m_List;
	TriggerSidebar* m_Sidebar;
	bool m_Condition;
};


class TriggerItemData : public wxTreeItemData, public sTrigger
{
public:
	TriggerItemData(TriggerSidebar* sidebar, const std::wstring& name, bool group)
		: m_Sidebar(sidebar), sTrigger(name), m_Group(group), m_CondCount(0), m_EffectCount(0) {}
	TriggerItemData(TriggerSidebar* sidebar, const sTrigger& trigger, bool group)
		: m_Sidebar(sidebar), sTrigger(trigger), m_Group(group), m_CondCount(0), m_EffectCount(0) {}

	TriggerSidebar* m_Sidebar;
	size_t m_CondCount, m_EffectCount;
	bool m_Group;
	std::list<int> m_BlockIndices, m_BlockEndIndices;	//index in sidebar list

	void AddBlock(const int block, const int index)
	{
		std::vector<int> copy = *logicBlocks;
		std::vector<bool> notCopy = *logicNots;
		copy.push_back(block);
		notCopy.push_back(false);

		logicBlocks = copy;
		logicNots = notCopy;
		m_BlockIndices.push_back(index);
	}
	void AddBlockEnd(const int block, const int index)
	{
		std::vector<int> copy = *logicBlockEnds;
		copy.push_back(block);
		logicBlockEnds = copy;
		m_BlockEndIndices.push_back(index);
	}

	void ResetBlockIndices()
	{
		std::vector<int> newLogicBlocks, newLogicBlockEnds;
		m_BlockIndices.clear();
		m_BlockEndIndices.clear();
		int conditionCount = 0;

		for (int i=0; i<m_Sidebar->m_ConditionPage->m_List->GetItemCount(); ++i)
		{
			if ( m_Sidebar->m_ConditionPage->m_List->GetItemText(i) == m_Sidebar->m_LogicBlockString )
			{
				newLogicBlocks.push_back(conditionCount);
				m_BlockIndices.push_back(i);
			}

			//Block ends belong to current condition, hence conditionCount-1
			else if ( m_Sidebar->m_ConditionPage->m_List->GetItemText(i) == m_Sidebar->m_LogicBlockEndString )
			{
				if ( conditionCount == 0 )
					newLogicBlockEnds.push_back(0);
				else
					newLogicBlockEnds.push_back(conditionCount-1);
				m_BlockEndIndices.push_back(i);
			}
			else
				++conditionCount;
		}
		logicBlocks = newLogicBlocks;
		logicBlockEnds = newLogicBlockEnds;
	}
};


void onTriggerParameter(void* data, std::wstring paramString, int parameter);


class TriggerBottomBar : public wxPanel
{
	enum {  ID_TimeEdit, ID_CondNameEdit, ID_EffectNameEdit, ID_TriggerNameEdit, ID_RunsEdit,
			ID_EffectChoice, ID_CondChoice, 
			ID_TimeRadio, ID_LogicRadio, 
			ID_NotCheck, ID_ActiveCheck, ID_LogicNotCheck };
	
public:
	enum { NO_VIEW, TRIGGER_VIEW, CONDITION_VIEW, EFFECT_VIEW, LOGIC_END_VIEW, LOGIC_VIEW };

	TriggerBottomBar(TriggerSidebar* sidebar, wxWindow* parent)
		: wxPanel(parent), m_Sidebar(sidebar)
	{
		 m_Sizer = new wxBoxSizer(wxHORIZONTAL);
		 SetSizer(m_Sizer);
		 m_DependentStatus = NO_VIEW;
	}
	
	int GetDependentStatus()
	{
		return m_DependentStatus;
	}
	void SetSpecs(std::vector<sTriggerSpec> conditions, std::vector<sTriggerSpec> effects)
	{
		m_ConditionSpecs = conditions;
		m_EffectSpecs = effects;
	}
	
	wxArrayString GetConditionNames()
	{
		wxArrayString ret;
		for ( size_t i = 0; i < m_ConditionSpecs.size(); ++i )
			ret.Add( wxString(m_ConditionSpecs[i].displayName.c_str()) );
		return ret;
	}
	wxArrayString GetEffectNames()
	{
		wxArrayString ret;
		for ( size_t i = 0; i < m_EffectSpecs.size(); ++i )
			ret.Add( wxString(m_EffectSpecs[i].displayName.c_str()) );
		return ret;
	}
	
	void onEffectChoice(wxCommandEvent& evt)
	{
		//Retrieve specification corresponding to selection
		if ( m_Sidebar->m_SelectedEffect != -1 )
		{
			std::vector<sTriggerSpec>::iterator it = std::find( m_EffectSpecs.begin(), 
				m_EffectSpecs.end(), std::wstring(evt.GetString()) );
			DisplayTriggerSpec(*it);
		}
	}

	void onCondChoice(wxCommandEvent& evt)
	{
		if ( m_Sidebar->m_SelectedCond != -1 )
		{	
			std::vector<sTriggerSpec>::iterator it = std::find( m_ConditionSpecs.begin(), 
				m_ConditionSpecs.end(), std::wstring(evt.GetString()) );
			DisplayTriggerSpec(*it);
		}
	}
	
	void onTimeEnter(wxCommandEvent& WXUNUSED(evt))
	{
		float fValue;
		std::wstringstream stream( std::wstring(m_TimeEdit->GetValue()) );
		stream >> fValue;
		
		if ( stream.fail() )
		{
			wxBell();
			return;
		}
		
		m_Sidebar->GetSelectedItemData()->timeValue = fValue;
		wxString value = wxString::Format(L"%.2f", fValue);
		m_TimeEdit->SetValue(value);
		m_Sidebar->UpdateEngineData();
	}

	void onConditionEnter(wxCommandEvent& WXUNUSED(evt))
	{
		if ( m_Sidebar->m_SelectedCond == -1 )
			return;

		std::vector<sTriggerCondition> conditions = *m_Sidebar->GetSelectedItemData()->conditions;
		int condition = m_Sidebar->GetConditionCount(m_Sidebar->m_SelectedCond);
		
		if ( condition == 0 )
			conditions[0].name = std::wstring( m_ConditionEdit->GetValue() );
		else
			conditions[condition-1] = std::wstring( m_ConditionEdit->GetValue() );
		
		m_Sidebar->GetSelectedItemData()->conditions = conditions;
		m_Sidebar->UpdateLists();
		m_Sidebar->UpdateEngineData();
	}
	void onEffectEnter(wxCommandEvent& WXUNUSED(evt))
	{
		if ( m_Sidebar->m_SelectedEffect== -1 )
			return;

		std::vector<sTriggerEffect> effects = *m_Sidebar->GetSelectedItemData()->effects;
		effects[m_Sidebar->m_SelectedEffect].name = std::wstring( m_EffectEdit->GetValue() );
		m_Sidebar->GetSelectedItemData()->effects = effects;
		m_Sidebar->UpdateLists();
		m_Sidebar->UpdateEngineData();
	}
	void onTriggerEnter(wxCommandEvent& WXUNUSED(evt))
	{
		TriggerItemData* data = m_Sidebar->GetSelectedItemData();
		if ( data == NULL || m_Sidebar->m_TriggerTree->GetSelection() == m_Sidebar->m_TriggerTree->GetRootItem() )
			return;
		
		wxString name = m_TriggerEdit->GetValue();
		data->name = std::wstring(name);
		m_Sidebar->m_TriggerTree->SetItemText(m_Sidebar->m_TriggerTree->GetSelection(), name);
		m_Sidebar->UpdateEngineData();
	}

	void onRunsEnter(wxCommandEvent& WXUNUSED(evt))
	{
		int iValue;
		std::wstringstream stream( std::wstring(m_RunsEdit->GetValue()) );
		stream >> iValue;
		
		if ( stream.fail() )
		{
			wxBell();
			return;
		}
		
		m_Sidebar->GetSelectedItemData()->maxRuns = iValue;
		m_Sidebar->UpdateEngineData();
	}
	
	void onLogicRadio(wxCommandEvent& evt)
	{
		if ( m_Sidebar->m_SelectedCond == -1 )
			return;
		std::vector<sTriggerCondition> conditions = *m_Sidebar->GetSelectedItemData()->conditions;
		int condition = m_Sidebar->GetConditionCount(m_Sidebar->m_SelectedCond);
		conditions[condition-1].linkLogic = evt.GetInt() + 1;
		
		m_Sidebar->GetSelectedItemData()->conditions = conditions;
		m_Sidebar->UpdateLists();
		m_Sidebar->UpdateEngineData();
	}
	void onActiveCheck(wxCommandEvent& evt)
	{
		m_Sidebar->GetSelectedItemData()->active = (evt.GetInt() == 1);
		m_Sidebar->UpdateEngineData();
	}
	void onNotCheck(wxCommandEvent& evt)
	{
		if ( m_Sidebar->m_SelectedCond == -1 )
			return;
		std::vector<sTriggerCondition> conditions = *m_Sidebar->GetSelectedItemData()->conditions;
		int condition = m_Sidebar->GetConditionCount(m_Sidebar->m_SelectedCond);
		bool value = (evt.GetInt() == 1);
		conditions[condition-1].negated = value;
		
		m_Sidebar->GetSelectedItemData()->conditions = conditions;
		m_Sidebar->UpdateLists();
		m_Sidebar->UpdateEngineData();
	}

	void onLogicNotCheck(wxCommandEvent& evt)
	{
		TriggerItemData* data = m_Sidebar->GetSelectedItemData();

		int logicIndex = m_Sidebar->GetLogicBlockCount(m_Sidebar->m_SelectedCond) - 1;
		std::vector<bool> nots = *data->logicNots;
		nots[logicIndex] = evt.IsChecked();
		data->logicNots = nots;
	}
	
	void DisplayTriggerSpec(const sTriggerSpec& spec)
	{
		if ( m_Sizer->Detach(m_ParameterSizer) )
		{
			m_ParameterSizer->DeleteWindows();
			delete m_ParameterSizer;
			//m_Sizer->Layout();
		//	Layout();
		}
		
		//m_ParameterSizer = new wxStaticBoxSizer(wxVERTICAL, this, L"Parameters");
		m_ParameterSizer = new wxBoxSizer(wxVERTICAL);
		wxBoxSizer* hRow = NULL;
		std::vector<sTriggerParameter> parameters = *spec.parameters;
		std::vector<std::wstring> stringParameters;
		int row = -1;
		
		//Set parameter data to new data if change is needed
		if ( m_Sidebar->m_Notebook->GetCurrentPage() == m_Sidebar->m_ConditionPage )
		{
			std::vector<sTriggerCondition> conditions = *m_Sidebar->GetSelectedItemData()->conditions;
			int condition = m_Sidebar->GetConditionCount(m_Sidebar->m_SelectedCond) - 1 ;
			if ( *conditions[condition].displayName != *spec.displayName )
			{
				std::vector<std::wstring> newParameters(parameters.size());
				conditions[condition].parameters = newParameters;
				conditions[condition].displayName = *spec.displayName;
				conditions[condition].functionName = *spec.functionName;
				m_Sidebar->GetSelectedItemData()->conditions = conditions;
				m_Sidebar->UpdateEngineData();
			}
			stringParameters = *(*m_Sidebar->GetSelectedItemData()->conditions)
													[m_Sidebar->m_SelectedCond].parameters;
		}
		else
		{
			std::vector<sTriggerEffect> effects = *m_Sidebar->GetSelectedItemData()->effects;
			if ( *effects[m_Sidebar->m_SelectedEffect].displayName != *spec.displayName )
			{
				std::vector<std::wstring> newParameters(parameters.size());
				effects[m_Sidebar->m_SelectedEffect].parameters = newParameters;
				effects[m_Sidebar->m_SelectedEffect].displayName = *spec.displayName;
				effects[m_Sidebar->m_SelectedEffect].functionName = *spec.functionName;
				m_Sidebar->GetSelectedItemData()->effects = effects;
				m_Sidebar->UpdateEngineData();
			}
			stringParameters = *(*m_Sidebar->GetSelectedItemData()->effects)
											[m_Sidebar->m_SelectedEffect].parameters;
		}
		
		//Add all parameters to sizer
		for ( std::vector<sTriggerParameter>::iterator it=parameters.begin(); it!=parameters.end(); ++it )
		{
			if ( it->row != row )
			{
				row = it->row;
				hRow = new wxBoxSizer(wxHORIZONTAL);
				m_ParameterSizer->Add(hRow);
			}
			
			
			if ( *it->windowType == std::wstring(L"text") )
			{
				hRow->Add( new wxStaticText(this, wxID_ANY, wxString( (*it->name).c_str() ), 
															wxPoint(it->xPos, it->yPos)) );
				wxTextCtrl* text = new TriggerSpecText(this, L"", wxDefaultPosition, wxSize(it->xSize, it->ySize), 
									it->parameterOrder, *it->inputType, &onTriggerParameter, this);
				
				hRow->Add( text, 0, wxLEFT, 5 );
				wxString fill( stringParameters[it->parameterOrder].c_str() );
				
				//Trim quotes
				if ( *it->inputType == L"string" && fill.size() > 0 )
				{
					fill.erase(0, 1);
					fill.erase(fill.size()-1, 1);
				}
				text->SetValue(fill);
			}

			else if ( *it->windowType == std::wstring(L"choice") )
			{
				qGetTriggerChoices qChoices(*spec.functionName + *it->name);
				qChoices.Post();
				std::vector<std::wstring> choices = *qChoices.choices;
				wxArrayString strings;

				for ( size_t i = 0; i < choices.size(); ++i )
					strings.Add( wxString(choices[i].c_str()) );

				hRow->Add( new wxStaticText(this, wxID_ANY, wxString( (*it->name).c_str() ), 
															wxPoint(it->xPos, it->yPos)) );
				wxChoice* choice = new TriggerSpecChoice( this, L"", wxDefaultPosition, wxSize(it->xSize, it->ySize), 
									strings, it->parameterOrder, &onTriggerParameter, this );

				hRow->Add(choice);
				choice->SetStringSelection( wxString(stringParameters[it->parameterOrder].c_str()) );
			}

			else if ( *it->windowType == std::wstring(L"entity_selector") )
			{
				hRow->Add( new wxStaticText(this, wxID_ANY, wxString((*it->name).c_str())) );
				hRow->Add( new TriggerEntitySelector(this, L"Select", wxDefaultPosition, 
					wxSize(it->xSize, it->ySize), it->parameterOrder, &onTriggerParameter, this) );
			}

			else if ( *it->windowType == std::wstring(L"point_placer") )
			{
				hRow->Add( new wxStaticText(this, wxID_ANY, wxString((*it->name).c_str())) );
				hRow->Add( new TriggerPointPlacer(this, wxDefaultPosition, wxSize(it->xSize, it->ySize),
							it->parameterOrder, &onTriggerParameter, this) );
			}
			else
			{
				wxFAIL_MSG(L"Invalid window type for trigger specification");
				row = -1;
				//do something else...
			}
		}
		
		//(If nothing was added, it won't be automatically delted)
		if ( row < 0 )
		{
			delete hRow;
			delete m_ParameterSizer;
		}
		else
			m_Sizer->Add(m_ParameterSizer, 0, wxLEFT, 5);

		m_Sizer->Layout();
		Layout();
	}
	
	void FillConditionData()
	{
		if ( m_Sidebar->m_SelectedCond == -1 )
			return;

		TriggerItemData* itemData = m_Sidebar->GetSelectedItemData();		
		int iCondition = m_Sidebar->GetConditionCount(m_Sidebar->m_SelectedCond);
		if ( iCondition <= 0 )
			return;
		sTriggerCondition condition = (*itemData->conditions)[iCondition-1];
		wxString display( (*condition.displayName).c_str() );
		m_ConditionEdit->SetValue( wxString(condition.name.c_str()) );

		if ( display != L"" )
		{
			std::vector<sTriggerSpec>::iterator it = std::find( m_ConditionSpecs.begin(), 
										m_ConditionSpecs.end(), std::wstring(display) );
			if ( it != m_ConditionSpecs.end() )
			{
				m_ConditionChoice->SetStringSelection(display);
				DisplayTriggerSpec(*it);
			}
		}
		else
			m_ConditionChoice->SetStringSelection(display);

		if ( condition.linkLogic == 0 || condition.linkLogic == 1 )
			m_LogicRadio->SetSelection(0);
		else
			m_LogicRadio->SetSelection(1);

		m_NotCheck->SetValue(condition.negated);
	}

	void FillEffectData()
	{
		TriggerItemData* itemData = m_Sidebar->GetSelectedItemData();
		sTriggerEffect effect = (*itemData->effects)[m_Sidebar->m_SelectedEffect];
		wxString display( (*effect.displayName).c_str() );

		m_EffectEdit->SetValue( wxString(effect.name.c_str()) );
		if ( display != L"" )
		{
			std::vector<sTriggerSpec>::iterator it = std::find( m_EffectSpecs.begin(), 
											m_EffectSpecs.end(), std::wstring(display) );
			if ( it != m_EffectSpecs.end() )
			{
				m_EffectChoice->SetStringSelection(display);
				DisplayTriggerSpec(*it);
			}
		}
		//m_TimeRadio->SetSelection(effect.loop);
		
		float timeVal = itemData->timeValue;
		wxString value = wxString::Format(L"%.2f", timeVal);
		m_TimeEdit->SetValue(value);
	}

	void FillTriggerData()
	{
		if ( m_DependentStatus != TRIGGER_VIEW )
			return;
		TriggerItemData* itemData = m_Sidebar->GetSelectedItemData();
		m_TriggerEdit->SetValue( wxString( (*itemData->name).c_str()) );
		m_ActiveCheck->SetValue(itemData->active);
		int runs = itemData->maxRuns;
		m_RunsEdit->SetValue( wxString( wxString::Format(L"%d", runs)) );

	}

	void FillLogicData()
	{
		std::vector<bool> nots = *m_Sidebar->GetSelectedItemData()->logicNots;
		m_LogicNotCheck->SetValue( nots[m_Sidebar->GetLogicBlockCount(m_Sidebar->m_SelectedCond)-1] );
	}


	void ToEffectView()
	{
		DestroyChildren();
		m_Sizer = new wxBoxSizer(wxHORIZONTAL);
		m_DependentSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxString(L"Trigger Editor"));
		SetSizer(m_Sizer, true);
		
		m_DependentSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxString(L"Trigger Editor"));
		wxStaticText* name = new wxStaticText(this, wxID_ANY, wxString(L"Name:"));
		wxStaticText* effect = new wxStaticText(this, wxID_ANY, wxString(L"Effect:"));
		m_EffectEdit = new wxTextCtrl(this, ID_EffectNameEdit, _T(""), wxDefaultPosition,
						wxSize(100, 18), wxTE_PROCESS_ENTER);

		wxArrayString effectNames = GetEffectNames();
		wxString radioChoice[] = { wxString(L"Delay"), wxString(L"Loop") };
		m_EffectChoice = new wxChoice(this, ID_EffectChoice, wxDefaultPosition, wxSize(100, 13), effectNames);

		m_TimeRadio = new wxRadioBox(this, ID_TimeRadio, _T("Execution type"),
			wxDefaultPosition, wxDefaultSize, 2, radioChoice, 2, wxRA_SPECIFY_COLS);

		wxStaticText* time = new wxStaticText(this, wxID_ANY, wxString(L"Time:"));
		m_TimeEdit = new wxTextCtrl(this, ID_TimeEdit, _T(""), wxDefaultPosition,
						wxSize(100, 18), wxTE_PROCESS_ENTER);

		wxBoxSizer* hNameHolder = new wxBoxSizer(wxHORIZONTAL);
		wxBoxSizer* hEffectHolder = new wxBoxSizer(wxHORIZONTAL);
		wxBoxSizer* hTimeHolder = new wxBoxSizer(wxHORIZONTAL);

		hNameHolder->Add(name);
		hNameHolder->Add(m_EffectEdit, 0, wxLEFT, 5);
		hEffectHolder->Add(effect);
		hEffectHolder->Add(m_EffectChoice, 0, wxLEFT, 5);
		hTimeHolder->Add(time);
		hTimeHolder->Add(m_TimeEdit, 0, wxLEFT, 5);

		m_DependentSizer->Add(hNameHolder, 0, wxTOP, 5);
		m_DependentSizer->Add(hEffectHolder, 0, wxTOP, 5);
		m_DependentSizer->Add(m_TimeRadio, 0, wxTOP | wxALIGN_CENTER, 10);
		m_DependentSizer->Add(hTimeHolder, 0, wxTOP | wxALIGN_CENTER, 5);

		m_Sizer->Add(m_DependentSizer);
		m_Sizer->Layout();
		Layout();
		m_DependentStatus = EFFECT_VIEW;
	}

	void ToConditionView()
	{
		DestroyChildren();
		m_Sizer = new wxBoxSizer(wxHORIZONTAL);
		m_DependentSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxString(L"Trigger Editor"));
		SetSizer(m_Sizer, true);
		
		wxStaticText* name = new wxStaticText(this, wxID_ANY, wxString(L"Name:"));
		wxStaticText* condition = new wxStaticText(this, wxID_ANY, wxString(L"Condition:"));
		m_ConditionEdit = new wxTextCtrl(this, ID_CondNameEdit, _T(""), wxDefaultPosition,
						wxSize(100, 18), wxTE_PROCESS_ENTER);

		wxArrayString conditionNames = GetConditionNames();
		wxString radioChoice[] = { wxString(L"And"), wxString(L"Or") };
		m_ConditionChoice = new wxChoice(this, ID_CondChoice, wxDefaultPosition,
											wxSize(100, 13), conditionNames);

		m_LogicRadio = new wxRadioBox(this, ID_LogicRadio, _T("Link logic:"),
			wxDefaultPosition, wxDefaultSize, 2, radioChoice, 2, wxRA_SPECIFY_COLS);

		m_NotCheck = new wxCheckBox(this, ID_NotCheck, wxString(L"Not "));

		wxBoxSizer* hNameHolder = new wxBoxSizer(wxHORIZONTAL);
		wxBoxSizer* hConditionHolder = new wxBoxSizer(wxHORIZONTAL);
		wxBoxSizer* hLogicHolder = new wxBoxSizer(wxHORIZONTAL);

		hNameHolder->Add(name);
		hNameHolder->Add(m_ConditionEdit, 0, wxLEFT | wxALIGN_CENTER, 5);
		hConditionHolder->Add(condition);
		hConditionHolder->Add(m_ConditionChoice, 0, wxLEFT | wxALIGN_CENTER, 5);
		hLogicHolder->Add(m_LogicRadio, 0, 0, 5);
		hLogicHolder->Add(m_NotCheck, 0, wxLEFT | wxALIGN_CENTER, 5);

		m_DependentSizer->Add(hNameHolder, 0, wxTOP, 5);
		m_DependentSizer->Add(hConditionHolder, 0, wxTOP, 5);
		m_DependentSizer->Add(hLogicHolder, 0, wxALIGN_CENTER | wxTOP, 10); 
		
		m_Sizer->Add(m_DependentSizer);
		m_Sizer->Layout();
		Layout();
		m_DependentStatus = CONDITION_VIEW;
	}

	void ToTriggerView()
	{
		DestroyChildren();
		m_Sizer = new wxBoxSizer(wxHORIZONTAL);
		m_DependentSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxString(L"Trigger Editor"));
		SetSizer(m_Sizer, true);
		
		wxStaticText* name = new wxStaticText(this, wxID_ANY, wxString(L"Name:"));
		m_TriggerEdit = new wxTextCtrl(this, ID_TriggerNameEdit, _T(""), wxDefaultPosition,
						wxSize(100, 18), wxTE_PROCESS_ENTER);
		
		wxStaticText* runs = new wxStaticText(this, wxID_ANY, wxString(L"Maximum runs:"));
		m_RunsEdit = new wxTextCtrl(this, ID_RunsEdit, wxString(L"-1"), wxDefaultPosition, wxSize(100, 18));
		m_ActiveCheck = new wxCheckBox(this, ID_ActiveCheck, wxString(L"Active: "));

		wxBoxSizer* nameHolder = new wxBoxSizer(wxHORIZONTAL), *runsHolder = new wxBoxSizer(wxHORIZONTAL);
		
		nameHolder->Add(name);
		nameHolder->Add(m_TriggerEdit, 0, wxLEFT, 5);
		runsHolder->Add(runs);
		runsHolder->Add(m_RunsEdit, 0, wxLEFT, 5);

		m_DependentSizer->Add(nameHolder);
		m_DependentSizer->Add(runsHolder, 0, wxTOP, 5);
		m_DependentSizer->Add(m_ActiveCheck, 0, wxTOP, 5);

		m_Sizer->Add(m_DependentSizer);
		m_Sizer->Layout();
		Layout();
		m_DependentStatus = TRIGGER_VIEW;
	}
	//void ToLogicEndView();
		
	void ToLogicView()
	{
		DestroyChildren();
		m_Sizer = new wxBoxSizer(wxHORIZONTAL);
		m_DependentSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxString(L"Trigger Editor"));
		SetSizer(m_Sizer, true);

		m_LogicNotCheck = new wxCheckBox(this, ID_LogicNotCheck, L"Not");
		m_DependentSizer->Add(m_LogicNotCheck);
		m_Sizer->Add(m_DependentSizer, 0, wxTOP | wxLEFT | wxALIGN_LEFT, 10);
		m_Sizer->Layout();
		Layout();
		m_DependentStatus = LOGIC_VIEW;
	}
	void ToNoView()
	{
		if ( m_DependentStatus == NO_VIEW )
			return;
		DestroyChildren();
		m_DependentStatus = NO_VIEW;
		m_Sidebar->m_ConditionPage->m_List->DeleteAllItems();
		m_Sidebar->m_EffectPage->m_List->DeleteAllItems();
	}
	
	TriggerSidebar* m_Sidebar;

private:

	wxBoxSizer* m_Sizer, *m_ParameterSizer;	
	wxStaticBoxSizer* m_DependentSizer; //dependent = effect/condition
	
	wxTextCtrl* m_TimeEdit, *m_ConditionEdit, *m_EffectEdit, *m_TriggerEdit, *m_RunsEdit;
	wxCheckBox* m_ActiveCheck, *m_NotCheck, *m_LogicNotCheck;
	wxChoice* m_ConditionChoice, *m_EffectChoice;
	wxRadioBox* m_LogicRadio, *m_TimeRadio, m_LogicEndRadio;

	std::vector<sTriggerSpec> m_ConditionSpecs, m_EffectSpecs;
	
	int m_DependentStatus;

	DECLARE_EVENT_TABLE();
};


BEGIN_EVENT_TABLE(TriggerBottomBar, wxPanel)
EVT_TEXT_ENTER(TriggerBottomBar::ID_TimeEdit, TriggerBottomBar::onTimeEnter)
EVT_TEXT_ENTER(TriggerBottomBar::ID_CondNameEdit, TriggerBottomBar::onConditionEnter)
EVT_TEXT_ENTER(TriggerBottomBar::ID_EffectNameEdit, TriggerBottomBar::onEffectEnter)
EVT_TEXT_ENTER(TriggerBottomBar::ID_TriggerNameEdit, TriggerBottomBar::onTriggerEnter)
EVT_TEXT_ENTER(TriggerBottomBar::ID_RunsEdit, TriggerBottomBar::onRunsEnter)
EVT_CHOICE(TriggerBottomBar::ID_EffectChoice, TriggerBottomBar::onEffectChoice)
EVT_CHOICE(TriggerBottomBar::ID_CondChoice, TriggerBottomBar::onCondChoice)

EVT_RADIOBOX(TriggerBottomBar::ID_LogicRadio, TriggerBottomBar::onLogicRadio)
EVT_CHECKBOX(TriggerBottomBar::ID_ActiveCheck, TriggerBottomBar::onActiveCheck)
EVT_CHECKBOX(TriggerBottomBar::ID_NotCheck, TriggerBottomBar::onNotCheck)
EVT_CHECKBOX(TriggerBottomBar::ID_LogicNotCheck, TriggerBottomBar::onLogicNotCheck)
//EVT_RADIOBOX(TriggerBotomBar::ID_TimeRadio, TriggerBottomBar::onTimeRadio)
END_EVENT_TABLE()


void TriggerTreeCtrl::onClick(wxMouseEvent& evt)
{
	if ( m_Sidebar->m_TriggerTree->GetSelection() == m_Sidebar->m_TriggerTree->GetRootItem() ||
											!m_Sidebar->m_TriggerTree->GetSelection() )
	{
		m_Sidebar->m_TriggerBottom->ToNoView();
	}
	else
	{
		m_Sidebar->m_TriggerBottom->ToTriggerView();
		m_Sidebar->m_TriggerBottom->FillTriggerData();
	}
	evt.Skip();
}

void TriggerListCtrl::onClick(wxMouseEvent& evt)
{
	evt.Skip();
	if ( m_Condition )
	{
		if ( m_Sidebar->m_SelectedCond < 0 )
			return;
		
		if ( m_Sidebar->m_ConditionPage->m_List->GetItemText(m_Sidebar->m_SelectedCond) 
													== m_Sidebar->m_LogicBlockEndString )
		{
			m_Sidebar->m_TriggerBottom->ToNoView();
		}
		else if ( m_Sidebar->m_ConditionPage->m_List->GetItemText(m_Sidebar->m_SelectedCond) 
													== m_Sidebar->m_LogicBlockString  )
		{
			m_Sidebar->m_TriggerBottom->ToLogicView();
			m_Sidebar->m_TriggerBottom->FillLogicData();
		}
		else
		{
			m_Sidebar->m_TriggerBottom->ToConditionView();
			m_Sidebar->m_TriggerBottom->FillConditionData();
		}
	}
	else
	{
		m_Sidebar->m_TriggerBottom->ToEffectView();
		if ( m_Sidebar->m_SelectedEffect != -1 )
			m_Sidebar->m_TriggerBottom->FillEffectData();
	}
	
}


TriggerEntitySelector::TriggerEntitySelector(TriggerBottomBar* parent, std::wstring label, 
		const wxPoint& pos, const wxSize& size, int parameter, callback func, void* data) 
		: wxPanel(parent), m_Parent(parent), m_Parameter(parameter), m_Callback(func), m_Data(data) 
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(MainSizer);
	MainSizer->Add( new wxButton(this, ID_SELECTION, wxString(label.c_str()), pos, size) );
	MainSizer->Add( new wxButton(this, ID_VIEW, L"View", pos, size) );
}
void TriggerEntitySelector::onViewClick(wxCommandEvent& WXUNUSED(evt))
{
	std::wstring handles;
	if ( m_Parent->m_Sidebar->m_Notebook->GetCurrentPage() == m_Parent->m_Sidebar->m_ConditionPage )
	{
		std::vector<sTriggerCondition> conditions = *m_Parent->m_Sidebar->GetSelectedItemData()->conditions;
		int condition = m_Parent->m_Sidebar->GetConditionCount(m_Parent->m_Sidebar->m_SelectedCond) - 1 ;
		std::vector<std::wstring> parameters = *conditions[condition].parameters;
		handles = parameters[m_Parameter];
	}
	else
	{
		std::vector<sTriggerEffect> effects = *m_Parent->m_Sidebar->GetSelectedItemData()->effects;
		int effect = m_Parent->m_Sidebar->m_SelectedEffect;
		std::vector<std::wstring> parameters = *effects[effect].parameters;
		handles = parameters[m_Parameter];
	}
	
	std::vector<ObjectID> IDList;
	size_t previous = handles.find(L"[")+1, current;
	
	//remove "]"
	if ( handles.size() )
		handles.erase(handles.size()-1);

	while ( (current = handles.find(L", ", previous)) != std::wstring::npos )
	{
		std::wstringstream toInt(handles.substr(previous, current - previous));
		int newID;
		toInt >> newID;
		IDList.push_back(newID);
		previous = current+1;
	}
	
	std::wstringstream toInt( handles.substr(previous) );
	int newID;
	toInt >> newID;
	IDList.push_back(newID);
	g_SelectedObjects = IDList;
	POST_MESSAGE(SetSelectionPreview, (g_SelectedObjects));
}
		
void TriggerSpecText::onTextEnter(wxCommandEvent& WXUNUSED(evt))
{
	std::wstring text( GetValue().wc_str() );
		
	if ( VerifyInput(text) )
		(*m_Callback)(m_Data, text, m_Parameter );
	else
		wxBell();
}

TriggerSpecChoice::TriggerSpecChoice(TriggerBottomBar* parent, std::wstring WXUNUSED(label), const wxPoint& pos, 
	const wxSize& size, const wxArrayString& strings, int parameter, callback func, void* data) 
	: wxChoice(parent, wxID_ANY, pos, size, strings), m_Callback(func), m_Data(data), 
											m_Parent(parent), m_Parameter(parameter)
	{ 
	}
void TriggerSpecChoice::onChoice(wxCommandEvent& evt)
{
	(*m_Callback)(m_Data, std::wstring( evt.GetString().wc_str() ), m_Parameter);
}
void onTriggerParameter(void* data, std::wstring paramString, int parameter)
{
	TriggerBottomBar* bottomBar = static_cast<TriggerBottomBar*>(data);
	
	if ( bottomBar->m_Sidebar->m_Notebook->GetSelection() == 0 )
	{
		if ( bottomBar->m_Sidebar->m_SelectedCond == -1 )
			return;
		std::vector<sTriggerCondition> conditions = *bottomBar->m_Sidebar->GetSelectedItemData()->conditions;
		std::vector<std::wstring> parameters = *conditions[bottomBar->m_Sidebar->m_SelectedCond].parameters;
	
		parameters[parameter] = paramString;
		conditions[bottomBar->m_Sidebar->m_SelectedCond].parameters = parameters;
		bottomBar->m_Sidebar->GetSelectedItemData()->conditions = conditions;
	}
	else
	{
		if ( bottomBar->m_Sidebar->m_SelectedEffect == -1 )
			return;
		std::vector<sTriggerEffect> effects = *bottomBar->m_Sidebar->GetSelectedItemData()->effects;
		std::vector<std::wstring> parameters = *effects[bottomBar->m_Sidebar->m_SelectedEffect].parameters;
	
		parameters[parameter] = paramString;
		effects[bottomBar->m_Sidebar->m_SelectedEffect].parameters = parameters;
		bottomBar->m_Sidebar->GetSelectedItemData()->effects = effects;
	}
	bottomBar->m_Sidebar->UpdateEngineData();
}

void onGroupPush(void* data)
{
	TriggerSidebar* sidebar = static_cast<TriggerSidebar*>(data);
	if ( !sidebar->m_TriggerTree->GetSelection())
		return;
	if ( !sidebar->IsGroupSelected() )
		return;

	wxString name = wxString::Format(L"Group %d", sidebar->m_GroupCount);
	wxTreeItemId ID = sidebar->m_TriggerTree->AppendItem( sidebar->m_TriggerTree->GetSelection(),
					name, -1, -1, new TriggerItemData(sidebar, std::wstring(name), true) );
	sidebar->m_TriggerTree->EnsureVisible(ID);
	++sidebar->m_GroupCount;

	sidebar->UpdateEngineData();
}

void onTriggerPush(void* data)
{
	TriggerSidebar* sidebar = static_cast<TriggerSidebar*>(data);
	wxTreeItemId ID = sidebar->m_TriggerTree->GetSelection();

	if ( !sidebar->IsGroupSelected() )
		ID = sidebar->m_TriggerTree->GetItemParent(ID);
	
	wxString name = wxString::Format(L"Trigger %d", sidebar->m_TriggerCount);
	TriggerItemData* itemData = new TriggerItemData(sidebar, std::wstring(name), false);
	itemData->group = std::wstring( sidebar->m_TriggerTree->GetItemText(ID) );
	
	ID = sidebar->m_TriggerTree->AppendItem(ID, name, -1, -1, itemData);
	sidebar->m_TriggerTree->Expand( sidebar->m_TriggerTree->GetRootItem() );
	
	++sidebar->m_TriggerCount;
	sidebar->m_TriggerTree->SelectItem(ID);
	sidebar->UpdateEngineData();
}

void onDeleteTreePush(void* data)
{
	TriggerSidebar* sidebar = static_cast<TriggerSidebar*>(data);
	if ( sidebar->m_TriggerTree->GetSelection() == sidebar->m_TriggerTree->GetRootItem() )
		return;

	if ( wxMessageBox( wxString(L"Are you sure you want to delete this item?"),
					wxString(L"Caution"), wxYES_NO ) == wxYES )
	{
		sidebar->m_TriggerTree->Delete(sidebar->m_TriggerTree->GetSelection());
		sidebar->m_TriggerTree->EnsureVisible( sidebar->m_TriggerTree->GetRootItem() );
		sidebar->m_TriggerBottom->FillTriggerData();
	}

	sidebar->UpdateEngineData();
}

void onConditionPush(void* data)
{
	TriggerSidebar* sidebar = static_cast<TriggerSidebar*>(data);
	if ( sidebar->IsGroupSelected() )
		return;

	sidebar->m_Notebook->SetSelection(0);
	TriggerItemData* itemData = sidebar->GetSelectedItemData();
	std::wstring name = std::wstring( wxString::Format(L"Condition %d", itemData->m_CondCount).wc_str() );
	
	if ( itemData->m_CondCount == 0 )
		sidebar->m_SelectedCond = 0;
	
	++itemData->m_CondCount;

	std::vector<sTriggerCondition> conditions = *itemData->conditions;
	conditions.push_back( sTriggerCondition(name) );
	itemData->conditions = conditions;
	
	long count = sidebar->m_ConditionPage->m_List->GetItemCount();

	sidebar->m_ConditionPage->m_List->InsertItem(count, wxString(name.c_str()) );
	sidebar->m_ConditionPage->m_List->EnsureVisible(count);
	sidebar->m_ConditionPage->m_List->SetItemState(count, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	
	sidebar->m_TriggerBottom->ToConditionView();	//Some data is not valid, so reset
	sidebar->m_TriggerBottom->FillConditionData();

	sidebar->UpdateEngineData();
}

void onEffectPush(void* data)
{
	TriggerSidebar* sidebar = static_cast<TriggerSidebar*>(data);
	if ( sidebar->IsGroupSelected() )
		return;

	sidebar->m_Notebook->SetSelection(1);
	TriggerItemData* itemData = sidebar->GetSelectedItemData();
	std::wstring name = std::wstring( wxString::Format(L"Effect %d", itemData->m_EffectCount).wc_str() );
	
	if ( itemData->m_EffectCount == 0 )
		sidebar->m_SelectedEffect = 0;
	++itemData->m_EffectCount;

	std::vector<sTriggerEffect> effects = *itemData->effects;
	effects.push_back( sTriggerEffect(name) );
	itemData->effects = effects;
	
	long count = sidebar->m_EffectPage->m_List->GetItemCount();
	sidebar->m_EffectPage->m_List->InsertItem(count, wxString(name.c_str()) );
	sidebar->m_EffectPage->m_List->EnsureVisible(count);
	sidebar->m_EffectPage->m_List->SetItemState(count, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	
	sidebar->m_TriggerBottom->ToEffectView();
	sidebar->m_TriggerBottom->FillEffectData();

	sidebar->UpdateEngineData();
}

void onDeleteBookPush(void* data)
{
	TriggerSidebar* sidebar = static_cast<TriggerSidebar*>(data);
	wxListCtrl* list = static_cast<TriggerPage*>(sidebar->m_Notebook->GetCurrentPage())->m_List;
	if ( !list )
		return;

	TriggerItemData* itemData = sidebar->GetSelectedItemData();
	//Is condition? -- valid selection?
	if ( list == sidebar->m_ConditionPage->m_List && sidebar->m_SelectedCond != -1 )
	{
		std::wstring text(sidebar->m_ConditionPage->m_List->GetItemText(sidebar->m_SelectedCond) ); 
		int conditionCount = sidebar->GetConditionCount(sidebar->m_SelectedCond); 

		if ( text == std::wstring(sidebar->m_LogicBlockString.wc_str()) )
		{
			std::vector<int> blocks = *itemData->logicBlocks;
			if ( conditionCount == 0 )
			{
				blocks.erase( std::find(blocks.begin(), blocks.end(), 0) );
				itemData->m_BlockIndices.erase( std::find( itemData->m_BlockIndices.begin(), 
												itemData->m_BlockIndices.end(), sidebar->m_SelectedCond ) );
			}
			else
			{
				blocks.erase( std::find(blocks.begin(), blocks.end(), conditionCount) );
				itemData->m_BlockIndices.erase( std::find( itemData->m_BlockIndices.begin(), 
								itemData->m_BlockIndices.end(), sidebar->m_SelectedCond ) );
			}
			itemData->logicBlocks = blocks;
		}
		
		else if ( text == std::wstring(sidebar->m_LogicBlockEndString.wc_str()) )
		{
			std::vector<int> blockEnds = *itemData->logicBlockEnds;
			if ( conditionCount == 0 )
			{
				blockEnds.erase( std::find(blockEnds.begin(), blockEnds.end(), 0) );
				itemData->m_BlockEndIndices.erase( std::find( itemData->m_BlockEndIndices.begin(), 
										itemData->m_BlockEndIndices.end(), sidebar->m_SelectedCond ) );
			}
			else
			{
				blockEnds.erase( std::find(blockEnds.begin(), blockEnds.end(), conditionCount-1) );
				itemData->m_BlockEndIndices.erase( std::find( itemData->m_BlockEndIndices.begin(), 
							itemData->m_BlockEndIndices.end(), sidebar->m_SelectedCond ) );
			}
			itemData->logicBlockEnds = blockEnds;
		}

		else
		{
			std::vector<sTriggerCondition> conditions = *itemData->conditions;
			conditions.erase( std::find(conditions.begin(), conditions.end(), text) );
			itemData->conditions = conditions;
		}

		list->DeleteItem( sidebar->m_SelectedCond );
		itemData->ResetBlockIndices();
		
		if ( sidebar->m_SelectedCond == list->GetItemCount() )
		{
			sidebar->m_SelectedCond = -1;
			sidebar->m_TriggerBottom->ToNoView();
		}
		else
			sidebar->m_TriggerBottom->FillConditionData();
	}
	
	else if ( list == sidebar->m_EffectPage->m_List && sidebar->m_SelectedEffect != -1)
	{
		std::vector<sTriggerEffect> effects = *itemData->effects;
		effects.erase( std::find( effects.begin(), effects.end(), std::wstring(
							list->GetItemText(sidebar->m_SelectedEffect)) ) );
		itemData->effects = effects;

		list->DeleteItem( sidebar->m_SelectedEffect );
		
		if ( itemData->effects.GetSize() == 0 || sidebar->m_SelectedEffect == list->GetItemCount() )
		{
			sidebar->m_SelectedEffect = -1;
			sidebar->m_TriggerBottom->ToNoView();
		}
		else
			sidebar->m_TriggerBottom->FillEffectData();
	}
	sidebar->UpdateLists();
	sidebar->UpdateEngineData();
}

void onLogicBlockPush(void* data)
{
	TriggerSidebar* sidebar = static_cast<TriggerSidebar*>(data);
	sidebar->m_Notebook->SetSelection(0);

	if ( sidebar->IsGroupSelected() )
		return;
	
	int limit = sidebar->m_SelectedCond;
	if ( sidebar->m_SelectedCond == -1 )
		limit = sidebar->m_ConditionPage->m_List->GetItemCount()-1;
	
	int conditionCount = sidebar->GetConditionCount(limit);
	if ( conditionCount == 0 )
	{
		sidebar->GetSelectedItemData()->AddBlock(0, 0);
		sidebar->UpdateLists();
		return;
	}

	sidebar->GetSelectedItemData()->AddBlock(conditionCount, limit);
	sidebar->UpdateLists();
	sidebar->m_TriggerBottom->ToLogicView();	//Some data is not valid, so reset
	sidebar->m_TriggerBottom->FillLogicData();
	sidebar->UpdateEngineData();
}

void onBlockEndPush(void* data)
{
	TriggerSidebar* sidebar = static_cast<TriggerSidebar*>(data);
	sidebar->m_Notebook->SetSelection(0);

	if ( sidebar->IsGroupSelected() )
		return;
	
	int limit = sidebar->m_SelectedCond;
	if ( sidebar->m_SelectedCond == -1 )
		limit = sidebar->m_ConditionPage->m_List->GetItemCount()-1;
	
	int conditionCount = sidebar->GetConditionCount(limit);
	if ( conditionCount == 0 )
	{
		sidebar->GetSelectedItemData()->AddBlockEnd(0, 0);
		sidebar->UpdateLists();
		return;
	}

	sidebar->GetSelectedItemData()->AddBlockEnd(conditionCount-1, limit);
	sidebar->UpdateLists();
}


TriggerSidebar::TriggerSidebar(ScenarioEditor& scenarioEditor, wxWindow* sidebarContainer, wxWindow* bottomBarContainer)
: Sidebar(scenarioEditor, sidebarContainer, bottomBarContainer), m_GroupCount(0), m_TriggerCount(0),
									m_SelectedCond(-1), m_SelectedEffect(-1)
{
	m_TriggerBottom = new TriggerBottomBar(this, bottomBarContainer);
	m_BottomBar = m_TriggerBottom;

	m_TriggerTree = new TriggerTreeCtrl(this, wxID_ANY, wxDefaultPosition,
						wxSize(140, 220), wxTR_HAS_BUTTONS | wxTR_EDIT_LABELS);
	m_TriggerTree->AddRoot(L"Triggers", -1, -1, new TriggerItemData(this, L"Triggers", true));
	m_TriggerTree->SelectItem( m_TriggerTree->GetRootItem() );
	m_TriggerTree->Expand( m_TriggerTree->GetRootItem() );

	
	wxBoxSizer* hHolder = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* vHolder = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* vButtons = new wxBoxSizer(wxVERTICAL);

	ActionButton* trigButton = new ActionButton( this, L"Trigger", &onTriggerPush,
									this, wxSize(50, 20) );
	ActionButton* groupButton = new ActionButton( this, L"Group", &onGroupPush,
											this, wxSize(50, 20) );
	ActionButton* deleteButton = new ActionButton( this, L"Delete", &onDeleteTreePush,
											this, wxSize(50, 20) );

	ActionButton* conditionButton = new ActionButton( this, L"Condition",
								&onConditionPush, this, wxSize(54, 20) );
	ActionButton* effectButton = new ActionButton( this, L"Effect",
								&onEffectPush, this, wxSize(50, 20) );
	ActionButton* bookDelete = new ActionButton( this, L"Delete",
								&onDeleteBookPush, this, wxSize(50, 20) );
	ActionButton* logicBlock = new ActionButton( this, L"Block",
								&onLogicBlockPush, this, wxSize(50, 20) );
	ActionButton* logicBlockEnd = new ActionButton( this, L"Block End",
								&onBlockEndPush, this, wxSize(50, 20) );


	m_LogicBlockString = wxString(L"--------------------");
	m_LogicBlockEndString = wxString(L"===========");
	wxStaticText* bottomTitle = new wxStaticText( this, wxID_ANY, _T("Conditions and Effects") );
	m_Notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition,
							wxSize(132, 210), wxNB_TOP);

	m_ConditionPage = new TriggerPage(m_Notebook, this, ID_CondList, wxString(L"Conditions"), true);
	m_EffectPage = new TriggerPage(m_Notebook, this, ID_EffectList, wxString(L"Effects"), false);

	m_Notebook->AddPage( m_ConditionPage, _T("Conditions") );
	m_Notebook->AddPage( m_EffectPage, _T("Effects") );
	m_Notebook->SetPageSize(wxSize(130, 240));

	vButtons->Add(trigButton);
	vButtons->Add(groupButton);
	vButtons->Add(deleteButton);
	vButtons->Add(conditionButton, 0, wxTOP, 190);
	vButtons->Add(effectButton);
	vButtons->Add(bookDelete);
	vButtons->Add(logicBlock);
	vButtons->Add(logicBlockEnd);

	vHolder->Add(m_TriggerTree, 0, wxTOP, 15);
	vHolder->Add(bottomTitle, 0, wxALIGN_CENTER | wxTOP, 5);
	vHolder->Add(m_Notebook, 0, wxTOP, 5 );

	hHolder->Add(vHolder);
	hHolder->Add(vButtons, 0, wxALIGN_CENTER);
	m_MainSizer->Add(hHolder);
}

int TriggerSidebar::GetConditionCount(int limit)
{
	int conditionCount = 0;
	wxListCtrl* list = m_ConditionPage->m_List;
	for ( int i = 0; i <= limit; ++i )
	{
		if ( list->GetItemText(i) != m_LogicBlockString && list->GetItemText(i) != m_LogicBlockEndString)
			++conditionCount;
	}
	return conditionCount;
}

int TriggerSidebar::GetLogicBlockCount(int limit)
{
	int logicCount = 0;
	wxListCtrl* list = m_ConditionPage->m_List;
	for ( int i = 0; i <= limit; ++i )
	{
		if ( list->GetItemText(i) == m_LogicBlockString )
			++logicCount;
	}
	return logicCount;
}
void TriggerSidebar::OnFirstDisplay()
{
	qGetTriggerData dataQuery;
	dataQuery.Post();
	m_TriggerBottom->SetSpecs(*dataQuery.conditions, *dataQuery.effects);
	
	//Add all loaded triggers to the tree
	const std::vector<sTriggerGroup> triggerGroups = *dataQuery.groups;
	std::vector<sTriggerGroup>::const_iterator it = std::find( triggerGroups.begin(), 
								triggerGroups.end(), std::wstring(L"Triggers") );

	if ( it != triggerGroups.end() )
	{
		wxTreeItemId invalid;	
		AddGroupTree(*it, invalid);
		m_TriggerTree->Expand( m_TriggerTree->GetRootItem() );
	}
}

void TriggerSidebar::AddGroupTree(const sTriggerGroup& group, wxTreeItemId parent)
{
	wxTreeItemId newID;
	wxString text( (*group.name).c_str() );

	//Make sure root item doesn't already exist
	if ( !parent && !m_TriggerTree->GetRootItem() )
		newID = m_TriggerTree->AddRoot(text, -1, -1, new TriggerItemData(this, *group.name, true));
	else if ( parent )
		newID = m_TriggerTree->AppendItem(parent, text);
	else
		newID = m_TriggerTree->GetRootItem();
	
	const std::vector<sTrigger> triggerBuf = *group.triggers;
	const std::vector<std::wstring> groupBuf = *group.children;
	
	for ( size_t i = 0; i < group.children.GetSize(); ++i )
		AddGroupTree( *std::find(m_TriggerGroups.begin(), m_TriggerGroups.end(), groupBuf[i]), newID );
	
	for ( size_t i = 0; i < group.triggers.GetSize(); ++i )
	{
		std::wstring trigName = *triggerBuf[i].name;
		size_t condMax = 0, effectMax = 0;

		//Make triggers start where user last left off
		if ( trigName.find(L"Trigger ") == 0 )
		{
			trigName.erase(0, 8);	//remove "Trigger "
			std::wstringstream toInt(trigName);
			size_t convert;
			toInt >> convert;
			++convert;

			if ( !toInt.fail() )
			{
				if ( convert > m_TriggerCount )
					m_TriggerCount = convert;
			}
		}
		std::vector<sTriggerCondition> conditions = *triggerBuf[i].conditions;
		std::vector<sTriggerEffect> effects = *triggerBuf[i].effects;

		for ( size_t j = 0; j < conditions.size(); ++j )
		{
			std::wstring condName = *conditions[j].name;
			
			if ( condName.find(L"Condition ") == 0 )
			{
				condName.erase(0, 10);
				std::wstringstream toInt(condName);
				size_t convert;
				toInt >> convert;
				++convert;

				if ( !toInt.fail() )
				{
					if ( convert > condMax )
						condMax = convert;
				}
			}
		}

		for ( size_t j = 0; j < effects.size(); ++j )
		{
			std::wstring effectName = *effects[j].name;

			if ( effectName .find(L"Effect ") == 0 )
			{
				effectName.erase(0, 7);
				std::wstringstream toInt(effectName);
				size_t convert;
				toInt >> convert;
				++convert;

				if ( !toInt.fail() )
				{
					if ( convert > effectMax )
						effectMax = convert;
				}
			}
		}
		TriggerItemData* newTriggerData = new TriggerItemData(this, triggerBuf[i], false);
		newTriggerData->m_CondCount = condMax;
		newTriggerData->m_EffectCount = effectMax;
		m_TriggerTree->AppendItem( newID, wxString( triggerBuf[i].name.c_str() ), -1, -1, newTriggerData);
	}
}


void TriggerSidebar::onPageChange(wxNotebookEvent& evt)
{
	if ( evt.GetSelection() == 0 )
	{
		m_TriggerBottom->ToConditionView();
		if ( m_SelectedCond != -1 )
			m_TriggerBottom->FillConditionData();
		return;
	}
	m_TriggerBottom->ToEffectView();
	if ( m_SelectedEffect != -1 )
		m_TriggerBottom->FillEffectData();
}
		
void TriggerSidebar::onTreeDrag(wxTreeEvent& WXUNUSED(evt))
{
	//evt.Allow();
}

void TriggerSidebar::onTreeNameChange(wxTreeEvent& evt)
{
	ToDerived( m_TriggerTree->GetItemData(evt.GetItem()) )->name = std::wstring(
														evt.GetLabel().wc_str());
	UpdateEngineData();
}

void TriggerSidebar::onTreeSelChange(wxTreeEvent& evt)
{
	//Prevent other triggers from trying to use previous data
	m_SelectedCond = -1;
	m_SelectedEffect = -1;

	if ( evt.GetItem() == m_TriggerTree->GetRootItem() )
	{
		m_TriggerBottom->ToNoView();
		return;
	}
	
	if ( m_TriggerBottom->GetDependentStatus() != TriggerBottomBar::TRIGGER_VIEW )
		m_TriggerBottom->ToTriggerView();
	m_TriggerBottom->FillTriggerData();
	
	UpdateLists();
}
void TriggerSidebar::UpdateLists()
{
	TriggerItemData* data = GetSelectedItemData();
	m_ConditionPage->m_List->Freeze();
	m_ConditionPage->m_List->DeleteAllItems();
	m_EffectPage->m_List->Freeze();
	m_EffectPage->m_List->DeleteAllItems();
	
	const Shareable<sTriggerCondition>* conditions = data->conditions.GetBuffer();
	for ( size_t i = 0; i < data->conditions.GetSize(); ++i )
	{
		m_ConditionPage->m_List->InsertItem( m_ConditionPage->m_List->
						GetItemCount(), wxString(conditions[i]->name.c_str()) );
	}
	
	const Shareable<sTriggerEffect>* effects = data->effects.GetBuffer();
	for ( size_t i = 0; i < data->effects.GetSize(); ++i )
	{
		m_EffectPage->m_List->InsertItem(m_EffectPage->m_List->GetItemCount(),
										wxString(effects[i]->name.c_str()) );
	}
	
	//These must be merged and sorted because adding them out-of-order screws up the list 
	std::list<LogicBlockHelper> sortedBlocks;
	std::list<int> blocks = data->m_BlockIndices, blockEnds = data->m_BlockEndIndices;
	
	for ( std::list<int>::iterator it = blocks.begin(); it != blocks.end(); ++it )
		sortedBlocks.push_back( LogicBlockHelper(*it, false) );
	for ( std::list<int>::iterator it = blockEnds.begin(); it != blockEnds.end(); ++it )
		sortedBlocks.push_back( LogicBlockHelper(*it, true) );

	sortedBlocks.sort();
	for ( std::list<LogicBlockHelper>::iterator it = sortedBlocks.begin(); it != sortedBlocks.end(); ++it )
	{
		if ( it->end )
			m_ConditionPage->m_List->InsertItem(it->index, m_LogicBlockEndString);
		else
			m_ConditionPage->m_List->InsertItem(it->index, m_LogicBlockString);
	}

	m_ConditionPage->m_List->Thaw();
	m_EffectPage->m_List->Thaw();
}

void TriggerSidebar::onCondSelect(wxListEvent& evt)
{
	m_SelectedCond = evt.GetIndex();
	//if ( m_TriggerBottom->GetDependentStatus() != TriggerBottomBar::CONDITION_VIEW )
	if ( m_ConditionPage->m_List->GetItemText(m_SelectedCond) 
												== m_LogicBlockEndString )
	{
		m_TriggerBottom->ToNoView();
	}
	else if ( m_ConditionPage->m_List->GetItemText(m_SelectedCond) 
												== m_LogicBlockString  )
	{
		m_TriggerBottom->ToLogicView();

		if ( m_SelectedCond != -1 )
			m_TriggerBottom->FillLogicData();
	}
	else
	{
		m_TriggerBottom->ToConditionView();
		if ( m_SelectedCond != -1 )
			m_TriggerBottom->FillConditionData();
	}
}
void TriggerSidebar::onEffectSelect(wxListEvent& evt)
{
	m_SelectedEffect = evt.GetIndex();
	//if ( m_TriggerBottom->GetDependentStatus() != TriggerBottomBar::EFFECT_VIEW )
		m_TriggerBottom->ToEffectView();
	m_TriggerBottom->FillEffectData();
}

bool TriggerSidebar::IsGroupSelected()
{
	if ( ToDerived( m_TriggerTree->GetItemData(m_TriggerTree->GetSelection()) )->m_Group )
		return true;
	return false;
}

sTrigger TriggerSidebar::CreateTrigger(TriggerItemData* data)
{
	sTrigger trigger;
	
	trigger.active = data->active;
	trigger.group = data->group;
	trigger.maxRuns = data->maxRuns;
	trigger.name = data->name;
	trigger.timeValue = data->timeValue;
	
	trigger.logicBlockEnds = data->logicBlockEnds;
	trigger.logicBlocks = data->logicBlocks;
	trigger.conditions = data->conditions;
	trigger.effects = data->effects;
	trigger.logicNots = data->logicNots;

	return trigger;
}

void TriggerSidebar::CreateGroup(std::vector<sTriggerGroup>& groupList, sTriggerGroup& parent, wxTreeItemId index)
{
	wxTreeItemIdValue cookie;
	std::vector<sTrigger> triggers;
	sTriggerGroup group( std::wstring(m_TriggerTree->GetItemText(index)) );
	group.parentName = parent.name;
	
	//Add this group to parent's child group
	std::vector<std::wstring> parentChildren = *parent.children;
	parentChildren.push_back(*group.parentName);
	parent.children = parentChildren;

	for ( wxTreeItemId ID = m_TriggerTree->GetFirstChild(index, cookie); ID.IsOk(); 
							ID = m_TriggerTree->GetNextChild(index, cookie) )
	{
		TriggerItemData* itemData = ToDerived( m_TriggerTree->GetItemData(ID) );
		if ( itemData->m_Group )
			CreateGroup(groupList, group, ID);
		else
			triggers.push_back( CreateTrigger(itemData) );
	}

	group.triggers = triggers;
	groupList.push_back(group);
}

void TriggerSidebar::UpdateEngineData()
{
	wxTreeItemIdValue cookie;
	wxTreeItemId root = m_TriggerTree->GetRootItem();
	
	//Find all root groups
	std::vector<sTriggerGroup> groups;
	std::vector<sTrigger> triggers;
	sTriggerGroup rootGroup(L"Triggers");

	for ( wxTreeItemId ID = m_TriggerTree->GetFirstChild(root, cookie); ID.IsOk(); 
							ID = m_TriggerTree->GetNextChild(root, cookie) )
	{
		TriggerItemData* itemData = ToDerived( m_TriggerTree->GetItemData(ID) );
		if ( itemData->m_Group )
			CreateGroup(groups, rootGroup, ID);
		else
			triggers.push_back( CreateTrigger(itemData) );
	}
	
	rootGroup.triggers = triggers;
	groups.push_back(rootGroup);
	POST_COMMAND( SetAllTriggers, (groups) );
}
TriggerItemData* TriggerSidebar::ToDerived(wxTreeItemData* data)
{
	return ( static_cast<TriggerItemData*>(data) );
}
TriggerItemData* TriggerSidebar::GetSelectedItemData()
{
	if ( !m_TriggerTree->GetSelection() )
		m_TriggerTree->SelectItem(m_TriggerTree->GetRootItem());
	return ToDerived( m_TriggerTree->GetItemData(m_TriggerTree->GetSelection()) );
	
}

