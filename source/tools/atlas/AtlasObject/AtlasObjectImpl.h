/* Copyright (C) 2021 Wildfire Games.
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

#include <string>

#ifdef _MSC_VER
 // Avoid complaints about unreachable code; the optimiser is realising
 // that some code is incapable of throwing, then warning about the catch
 // block that will never be executed.
 #pragma warning(disable: 4702)
 #include <map>
 #pragma warning(default: 4702)
#else
 #include <map>
#endif

// AtNode is an immutable tree node, with a string and and multimap of children.
class AtNode
{
	friend class AtSmartPtr<AtNode>;
	friend class AtSmartPtr<const AtNode>;

public:
	typedef AtSmartPtr<const AtNode> Ptr;

	AtNode() : m_Refcount(0) {}
	explicit AtNode(const AtNode* n) { *this = *n; m_Refcount = 0; }
	explicit AtNode(const char* text) : m_Refcount(0), m_Value(text) {}

	// Create a new AtNode (since AtNodes are immutable, so it's not possible
	// to just change this one), with the relevant alterations to its content.
	const AtNode::Ptr setValue(const char* value) const;
	const AtNode::Ptr addChild(const char* key, const AtNode::Ptr &data) const;
	const AtNode::Ptr setChild(const char* key, const AtNode::Ptr &data) const;
	const AtNode::Ptr unsetChild(const char* key) const;
	const AtNode::Ptr addOverlay(const AtNode::Ptr &data) const;
	const AtIter getChild(const char* key) const;

	// Check recursively for any 'value' data
	bool hasContent() const;

//private:	// (but not actually private, since I'm still too lazy to waste
			// time with dozens of friends)

	std::string m_Value;

	typedef std::multimap<std::string, AtNode::Ptr> child_maptype;
	typedef std::pair<std::string, AtNode::Ptr> child_pairtype;

	child_maptype m_Children;

private:
	mutable unsigned int m_Refcount;
};

// Implementation of AtIter
class AtIterImpl
{
	friend class AtSmartPtr<AtIterImpl>;

public:
	AtIterImpl() : m_Refcount(0) {}

	AtIterImpl(AtNode::child_maptype::const_iterator it, AtNode::child_maptype::const_iterator up)
		: m_Refcount(0), iter(it), iter_upperbound(up) {}

	AtNode::child_maptype::const_iterator iter, iter_upperbound;

private:
	mutable unsigned int m_Refcount;
};
