/* Copyright (C) 2015 Wildfire Games.
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
		if (ptr) ++ptr->refcount;				\
	}											\
												\
	template<> void AtSmartPtr<T>::dec_ref()	\
	{											\
		if (ptr && --ptr->refcount == 0)		\
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
	if (p)
		return p->iter->second->getChild(key);
	else
		return AtIter();
}

AtIter::operator const wchar_t* () const
{
	if (p)
		return p->iter->second->value.c_str();
	else
		return L"";
}

//AtIter::operator const AtObj () const
const AtObj AtIter::operator * () const
{
	if (p)
	{
		AtObj ret;
		ret.p = p->iter->second;
		return ret;
	}
	else
		return AtObj();
}


AtIter& AtIter::operator ++ ()
{
	assert(p);

	// Increment the internal iterator, and stop if we've run out children
	// to iterate over.
	if (p && ++p->iter == p->iter_upperbound)
		p = NULL;

	return *this;
}

bool AtIter::defined() const
{
	return (bool)p;
}

bool AtIter::hasContent() const
{
	if (!p)
		return false;

	if (! p->iter->second)
		return false;

	return p->iter->second->hasContent();
}

size_t AtIter::count() const
{
	if (!p)
		return 0;

	return std::distance(p->iter, p->iter_upperbound);
}

//////////////////////////////////////////////////////////////////////////

const AtIter AtObj::operator [] (const char* key) const
{
	if (p)
		return p->getChild(key);
	else
		// This object doesn't exist, so return another object that doesn't exist
		return AtIter();
}

AtObj::operator const wchar_t* () const
{
	if (p)
		return p->value.c_str();
	else
		return L"";
}

double AtObj::getDouble() const
{
	double val = 0;
	if (p)
	{
		std::wstringstream s;
		s << p->value;
		s >> val;
	}
	return val;
}

int AtObj::getInt() const
{
	int val = 0;
	if (p)
	{
		std::wstringstream s;
		s << p->value;
		s >> val;
	}
	return val;
}

void AtObj::add(const char* key, AtObj& data)
{
	if (!p)
		p = new AtNode();

	p = p->addChild(key, data.p);
}

void AtObj::add(const char* key, const wxString& value)
{
	add(key, value.wc_str());
}

void AtObj::add(const char* key, const wchar_t* value)
{
	const AtNode* o = new AtNode(value);

	if (!p)
		p = new AtNode();

	p = p->addChild(key, AtNode::Ptr(o));
}

void AtObj::set(const char* key, AtObj& data)
{
	if (!p)
		p = new AtNode();

	p = p->setChild(key, data.p);
}

void AtObj::set(const char* key, const wxString& value)
{
	set(key, value.wc_str());
}

void AtObj::set(const char* key, const wchar_t* value)
{
	const AtNode* o = new AtNode(value);

	if (!p)
		p = new AtNode();

	p = p->setChild(key, AtNode::Ptr(o));
}

void AtObj::setBool(const char* key, bool value)
{
	AtNode* o = new AtNode(value ? L"true" : L"false");
	o->children.insert(AtNode::child_pairtype("@boolean", AtNode::Ptr(new AtNode())));

	if (!p)
		p = new AtNode();

	p = p->setChild(key, AtNode::Ptr(o));
}

void AtObj::setDouble(const char* key, double value)
{
	std::wstringstream str;
	str << value;
	AtNode* o = new AtNode(str.str().c_str());
	o->children.insert(AtNode::child_pairtype("@number", AtNode::Ptr(new AtNode())));

	if (!p)
		p = new AtNode();

	p = p->setChild(key, AtNode::Ptr(o));
}

void AtObj::setInt(const char* key, int value)
{
	std::wstringstream str;
	str << value;
	AtNode* o = new AtNode(str.str().c_str());
	o->children.insert(AtNode::child_pairtype("@number", AtNode::Ptr(new AtNode())));

	if (!p)
		p = new AtNode();

	p = p->setChild(key, AtNode::Ptr(o));
}

void AtObj::setString(const wchar_t* value)
{
	if (!p)
		p = new AtNode();

	p = p->setValue(value);
}

void AtObj::addOverlay(AtObj& data)
{
	if (!p)
		p = new AtNode();

	p = p->addOverlay(data.p);
}

bool AtObj::hasContent() const
{
	if (!p)
		return false;

	return p->hasContent();
}

//////////////////////////////////////////////////////////////////////////

const AtIter AtNode::getChild(const char* key) const
{
	// Find the range of matching children
	AtNode::child_maptype::const_iterator it = children.lower_bound(key);
	AtNode::child_maptype::const_iterator it_upper = children.upper_bound(key);

	if (it == it_upper) // No match found
		return AtIter();

	AtIter obj;
	obj.p = new AtIterImpl(it, it_upper);
	return obj;
}

bool AtNode::hasContent() const
{
	if (value.length())
		return true;

	for (child_maptype::const_iterator it = children.begin(); it != children.end(); ++it)
		if (it->second && it->second->hasContent())
			return true;

	return false;
}

const AtNode::Ptr AtNode::setValue(const wchar_t* value) const
{
	AtNode* newNode = new AtNode();
	newNode->children = children;
	newNode->value = value;
	return AtNode::Ptr(newNode);
}

const AtNode::Ptr AtNode::setChild(const char* key, const AtNode::Ptr &data) const
{
	AtNode* newNode = new AtNode(this);
	newNode->children.erase(key);
	newNode->children.insert(AtNode::child_pairtype(key, data));
	return AtNode::Ptr(newNode);
}

const AtNode::Ptr AtNode::addChild(const char* key, const AtNode::Ptr &data) const
{
	AtNode* newNode = new AtNode(this);
	newNode->children.insert(AtNode::child_pairtype(key, data));
	return AtNode::Ptr(newNode);
}

const AtNode::Ptr AtNode::addOverlay(const AtNode::Ptr &data) const
{
	AtNode* newNode = new AtNode(this);

	// Delete old childs that are also in the overlay
	for (AtNode::child_maptype::const_iterator it = data->children.begin(); it != data->children.end(); ++it)
		newNode->children.erase(it->first);

	// Add the overlay childs back in
	for (AtNode::child_maptype::const_iterator it = data->children.begin(); it != data->children.end(); ++it)
		newNode->children.insert(*it);

	return AtNode::Ptr(newNode);
}
//////////////////////////////////////////////////////////////////////////

AtObj AtlasObject::TrimEmptyChildren(AtObj& obj)
{
	AtObj ret;

	for (AtNode::child_maptype::const_iterator it = obj.p->children.begin();
			it != obj.p->children.end(); ++it)
	{
		if (it->second && it->second->hasContent())
		{
			AtObj node;
			node.p = it->second;
			ret.add(it->first.c_str(), node);
		}
	}

	return ret;
}
