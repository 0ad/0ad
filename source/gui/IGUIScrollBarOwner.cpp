/*
IGUIScrollBarOwner
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx.h"
#include "GUI.h"

using namespace std;

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
IGUIScrollBarOwner::IGUIScrollBarOwner()
{
}

IGUIScrollBarOwner::~IGUIScrollBarOwner()
{
	// Delete scroll-bars
	vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		delete *it;
	}
}

void IGUIScrollBarOwner::ResetStates()
{
	IGUIObject::ResetStates();
	
	vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		(*it)->SetBarPressed(false);
	}
}

void IGUIScrollBarOwner::AddScrollBar(IGUIScrollBar * scrollbar)
{
	scrollbar->SetHostObject(this);
	m_ScrollBars.push_back(scrollbar);
}

const SGUIScrollBarStyle & IGUIScrollBarOwner::GetScrollBarStyle(const CStr &style) const
{
	if (!GetGUI())
	{
		// TODO Gee: Output in log
		return SGUIScrollBarStyle();
	}
	
	if (GetGUI()->m_ScrollBarStyles.count(style) == 0)
	{
		// TODO Gee: Output in log
		return SGUIScrollBarStyle();
	}

 	return GetGUI()->m_ScrollBarStyles.find(style)->second;
}

void IGUIScrollBarOwner::HandleMessage(const SGUIMessage &Message)
{
	vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		(*it)->HandleMessage(Message);
	}
}

void IGUIScrollBarOwner::Draw() 
{
	vector<IGUIScrollBar*>::iterator it;
	for (it=m_ScrollBars.begin(); it!=m_ScrollBars.end(); ++it)
	{
		(*it)->Draw();
	}
}