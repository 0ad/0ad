/* Copyright (C) 2019 Wildfire Games.
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

#include "AtlasObject.h"
#include "AtlasObjectImpl.h"

#include <assert.h>
#include <sstream>

#include <wx/string.h>

#define ATSMARTPTR_IMPL(T) \
	template<> void AtSmartPtr<T>::inc_ref()	\
	{											\
		if (ptr) ++ptr->m_Refcount;				\
	}											\
												\
	template<> void AtSmartPtr<T>::dec_ref()	\
	{											\
		if (ptr && --ptr->m_Refcount == 0)		\
			delete ptr;							\
	}	// (Don't set ptr=NULL, since it should never be possible for an
		// unreferenced pointer to exist; I would rather see it die in debug
		// mode if that ever happens, instead of just silently ignoring the error.)


ATSMARTPTR_IMPL(AtNode)
ATSMARTPTR_IMPL(const AtNode)
ATSMARTPTR_IMPL(AtIterImpl)


//////////////////////////////////////////////////////////////////////////

const AtIter AtIter::operator [] (const char* key) const
{
	if (m_Impl)
		return m_Impl->iter->second->getChild(key);
	else
		return AtIter();
}

AtIter::operator const wchar_t* () const
{
	if (m_Impl)
		return m_Impl->iter->second->m_Value.c_str();
	else
		return L"";
}

//AtIter::operator const AtObj () const
const AtObj AtIter::operator * () const
{
	if (m_Impl)
	{
		AtObj ret;
		ret.m_Node = m_Impl->iter->second;
		return ret;
	}
	else
		return AtObj();
}


AtIter& AtIter::operator ++ ()
{
	assert(m_Impl);

	// Increment the internal iterator, and stop if we've run out children
	// to iterate over.
	if (m_Impl && ++m_Impl->iter == m_Impl->iter_upperbound)
		m_Impl = nullptr;

	return *this;
}

bool AtIter::defined() const
{
	return static_cast<bool>(m_Impl);
}

bool AtIter::hasContent() const
{
	if (!m_Impl)
		return false;

	if (!m_Impl->iter->second)
		return false;

	return m_Impl->iter->second->hasContent();
}

size_t AtIter::count() const
{
	if (!m_Impl)
		return 0;

	return std::distance(m_Impl->iter, m_Impl->iter_upperbound);
}

//////////////////////////////////////////////////////////////////////////

const AtIter AtObj::operator [] (const char* key) const
{
	if (m_Node)
		return m_Node->getChild(key);
	else
		// This object doesn't exist, so return another object that doesn't exist
		return AtIter();
}

AtObj::operator const wchar_t* () const
{
	if (m_Node)
		return m_Node->m_Value.c_str();
	else
		return L"";
}

double AtObj::getDouble() const
{
	double val = 0;
	if (m_Node)
	{
		std::wstringstream s;
		s << m_Node->m_Value;
		s >> val;
	}
	return val;
}

int AtObj::getInt() const
{
	int val = 0;
	if (m_Node)
	{
		std::wstringstream s;
		s << m_Node->m_Value;
		s >> val;
	}
	return val;
}

void AtObj::add(const char* key, AtObj& data)
{
	if (!m_Node)
		m_Node = new AtNode();

	m_Node = m_Node->addChild(key, data.m_Node);
}

void AtObj::add(const char* key, const wxString& value)
{
	add(key, value.wc_str());
}

void AtObj::add(const char* key, const wchar_t* value)
{
	const AtNode* o = new AtNode(value);

	if (!m_Node)
		m_Node = new AtNode();

	m_Node = m_Node->addChild(key, AtNode::Ptr(o));
}

void AtObj::set(const char* key, AtObj& data)
{
	if (!m_Node)
		m_Node = new AtNode();

	m_Node = m_Node->setChild(key, data.m_Node);
}

void AtObj::set(const char* key, const wxString& value)
{
	set(key, value.wc_str());
}

void AtObj::set(const char* key, const wchar_t* value)
{
	const AtNode* o = new AtNode(value);

	if (!m_Node)
		m_Node = new AtNode();

	m_Node = m_Node->setChild(key, AtNode::Ptr(o));
}

void AtObj::setBool(const char* key, bool value)
{
	AtNode* o = new AtNode(value ? L"true" : L"false");
	o->m_Children.insert(AtNode::child_pairtype("@boolean", AtNode::Ptr(new AtNode())));

	if (!m_Node)
		m_Node = new AtNode();

	m_Node = m_Node->setChild(key, AtNode::Ptr(o));
}

void AtObj::setDouble(const char* key, double value)
{
	std::wstringstream str;
	str << value;
	AtNode* o = new AtNode(str.str().c_str());
	o->m_Children.insert(AtNode::child_pairtype("@number", AtNode::Ptr(new AtNode())));

	if (!m_Node)
		m_Node = new AtNode();

	m_Node = m_Node->setChild(key, AtNode::Ptr(o));
}

void AtObj::setInt(const char* key, int value)
{
	std::wstringstream str;
	str << value;
	AtNode* o = new AtNode(str.str().c_str());
	o->m_Children.insert(AtNode::child_pairtype("@number", AtNode::Ptr(new AtNode())));

	if (!m_Node)
		m_Node = new AtNode();

	m_Node = m_Node->setChild(key, AtNode::Ptr(o));
}

void AtObj::setString(const wchar_t* value)
{
	if (!m_Node)
		m_Node = new AtNode();

	m_Node = m_Node->setValue(value);
}

void AtObj::addOverlay(AtObj& data)
{
	if (!m_Node)
		m_Node = new AtNode();

	m_Node = m_Node->addOverlay(data.m_Node);
}

bool AtObj::hasContent() const
{
	if (!m_Node)
		return false;

	return m_Node->hasContent();
}

//////////////////////////////////////////////////////////////////////////

const AtIter AtNode::getChild(const char* key) const
{
	// Find the range of matching children
	AtNode::child_maptype::const_iterator it = m_Children.lower_bound(key);
	AtNode::child_maptype::const_iterator it_upper = m_Children.upper_bound(key);

	if (it == it_upper) // No match found
		return AtIter();

	AtIter obj;
	obj.m_Impl = new AtIterImpl(it, it_upper);
	return obj;
}

bool AtNode::hasContent() const
{
	if (m_Value.length())
		return true;

	for (const child_maptype::value_type& child : m_Children)
		if (child.second && child.second->hasContent())
			return true;

	return false;
}

const AtNode::Ptr AtNode::setValue(const wchar_t* value) const
{
	AtNode* newNode = new AtNode();
	newNode->m_Children = m_Children;
	newNode->m_Value = value;
	return AtNode::Ptr(newNode);
}

const AtNode::Ptr AtNode::setChild(const char* key, const AtNode::Ptr &data) const
{
	AtNode* newNode = new AtNode(this);
	newNode->m_Children.erase(key);
	newNode->m_Children.insert(AtNode::child_pairtype(key, data));
	return AtNode::Ptr(newNode);
}

const AtNode::Ptr AtNode::addChild(const char* key, const AtNode::Ptr &data) const
{
	AtNode* newNode = new AtNode(this);
	newNode->m_Children.insert(AtNode::child_pairtype(key, data));
	return AtNode::Ptr(newNode);
}

const AtNode::Ptr AtNode::addOverlay(const AtNode::Ptr &data) const
{
	AtNode* newNode = new AtNode(this);

	// Delete old childs that are also in the overlay
	for (AtNode::child_maptype::const_iterator it = data->m_Children.begin(); it != data->m_Children.end(); ++it)
		newNode->m_Children.erase(it->first);

	// Add the overlay childs back in
	for (AtNode::child_maptype::const_iterator it = data->m_Children.begin(); it != data->m_Children.end(); ++it)
		newNode->m_Children.insert(*it);

	return AtNode::Ptr(newNode);
}
//////////////////////////////////////////////////////////////////////////

AtObj AtlasObject::TrimEmptyChildren(AtObj& obj)
{
	AtObj ret;

	for (const AtNode::child_maptype::value_type& child : obj.m_Node->m_Children)
	{
		if (child.second && child.second->hasContent())
		{
			AtObj node;
			node.m_Node = child.second;
			ret.add(child.first.c_str(), node);
		}
	}

	return ret;
}
