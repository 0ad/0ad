/*
IGUIScrollBarOwner
*/

#include "precompiled.h"
#include "GUI.h"


//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
IGUIScrollBarOwner::IGUIScrollBarOwner()
{
}

IGUIScrollBarOwner::~IGUIScrollBarOwner()
{
	// Delete scroll-bars
	std::vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		delete *it;
	}
}

void IGUIScrollBarOwner::ResetStates()
{
	IGUIObject::ResetStates();
	
	std::vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		(*it)->SetBarPressed(false);
	}
}

void IGUIScrollBarOwner::AddScrollBar(IGUIScrollBar * scrollbar)
{
	scrollbar->SetHostObject(this);
	scrollbar->SetGUI(GetGUI());
	m_ScrollBars.push_back(scrollbar);
}

SGUIScrollBarStyle * IGUIScrollBarOwner::GetScrollBarStyle(const CStr& style) const
{
	if (!GetGUI())
	{
		// TODO Gee: Output in log
		return NULL;
	}
	
	if (GetGUI()->m_ScrollBarStyles.count(style) == 0)
	{
		// TODO Gee: Output in log
		return NULL;
	}

 	return &((CGUI*)GetGUI())->m_ScrollBarStyles.find(style)->second;
}

void IGUIScrollBarOwner::HandleMessage(const SGUIMessage &Message)
{
	std::vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		(*it)->HandleMessage(Message);
	}
}

void IGUIScrollBarOwner::Draw() 
{
	std::vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		(*it)->Draw();
	}
}
