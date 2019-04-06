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

// Public interface to almost all of AtlasObject.
// (See AtlasObjectText for the rest of it).
//
// Tries to include as few headers as possible, to minimise its impact
// on compile times.

#ifndef INCLUDED_ATLASOBJECT
#define INCLUDED_ATLASOBJECT

#if defined(_WIN32)
# define _CRT_SECURE_NO_WARNINGS // Disable deprecation warning in VS2008
#endif

#include <wchar.h> // for wchar_t
#include <string>

class wxString;

//////////////////////////////////////////////////////////////////////////
// Mostly-private bits:

// Helper class to let us define a conversion operator only for AtSmartPtr<not const>
template<class ConstSmPtr, class T> struct ConstCastHelper { operator ConstSmPtr (); };
template<class ConstSmPtr, class T> struct ConstCastHelper<ConstSmPtr, const T> { };

// Simple reference-counted pointer. class T must contain a reference count,
// initialised to 0. An external implementation (in AtlasObjectImpl.cpp)
// provides the inc_ref and dec_ref methods, so that this header file doesn't
// need to know their implementations.
template<class T> class AtSmartPtr : public ConstCastHelper<AtSmartPtr<const T>, T>
{
	friend struct ConstCastHelper<AtSmartPtr<const T>, T>;
private:
	void inc_ref();
	void dec_ref();
	T* ptr;
public:
	// Constructors
	AtSmartPtr() : ptr(NULL) {}
	explicit AtSmartPtr(T* p) : ptr(p) { inc_ref(); }
	// Copy constructor
	AtSmartPtr(const AtSmartPtr<T>& r) : ptr(r.ptr) { inc_ref(); }
	// Assignment operators
	AtSmartPtr<T>& operator=(T* p) { dec_ref(); ptr = p; inc_ref(); return *this; }
	AtSmartPtr<T>& operator=(const AtSmartPtr<T>& r) { if (&r != this) { dec_ref(); ptr = r.ptr; inc_ref(); } return *this; }
	// Destructor
	~AtSmartPtr() { dec_ref(); }
	// Allow conversion from non-const T* to const T*
	//operator AtSmartPtr<const T> () { return AtSmartPtr<const T>(ptr); } // (actually provided by ConstCastHelper)
	// Override ->
	T* operator->() const { return ptr; }
	// Test whether the pointer is pointing to anything
	explicit operator bool() const { return ptr != nullptr; }
};

template<class ConstSmPtr, class T>
ConstCastHelper<ConstSmPtr, T>::operator ConstSmPtr ()
{
	return ConstSmPtr(static_cast<AtSmartPtr<T>*>(this)->ptr);
}

// A few required declarations
class AtObj;
class AtNode;
class AtIterImpl;


//////////////////////////////////////////////////////////////////////////
// Public bits:


// AtIter is an iterator over AtObjs - use it like:
//
//     for (AtIter thing = whatever["thing"]; thing.defined(); ++thing)
//         DoStuff(thing);
//
// to handle XML data like:
//
//   <whatever>
//     <thing>Stuff 1</thing>
//     <thing>Stuff 2</thing>
//   </whatever>

class AtIter
{
public:
	// Increment the iterator; or make it undefined, if there weren't any
	// AtObjs left to iterate over
	AtIter& operator++ ();
	// Return whether this iterator has an AtObj to point to
	bool defined() const;
	// Return whether this iterator is pointing to a non-contentless AtObj
	bool hasContent() const;
	// Return the number of AtObjs that will be iterated over (including the current one)
	size_t count() const;

	// Return an iterator to the children matching 'key'. (That is, children
	// of the AtObj currently pointed to by this iterator)
	const AtIter operator[] (const char* key) const;

	// Return the AtObj currently pointed to by this iterator
	const AtObj operator* () const;

	// Return the string value of the AtObj currently pointed to by this iterator
	operator const wchar_t* () const;

	// Private implementation. (But not 'private:', because it's a waste of time
	// adding loads of friend functions)
	AtSmartPtr<AtIterImpl> m_Impl;
};


class AtObj
{
public:
	AtObj() {}
	AtObj(const AtObj& r) : m_Node(r.m_Node) {}

	// Return an iterator to the children matching 'key'
	const AtIter operator[] (const char* key) const;

	// Return the string value of this object
	operator const wchar_t* () const;

	// Return the floating point value of this object
	double getDouble() const;

	// Return the integer value of this object
	int getInt() const;

	// Check whether the object contains anything (even if those things are empty)
	bool defined() const { return static_cast<bool>(m_Node); }

	// Check recursively whether there's actually any non-empty data in the object
	bool hasContent() const;

	// Add or set a child. The wchar_t* and wxString& versions create a new
	// AtObj with the appropriate string value, then use that as the child.
	//
	// These alter the AtObj's internal pointer, and the pointed-to data is
	// never actually altered. Copies of this AtObj (including copies stored
	// inside other AtObjs) will not be affected.
	void add(const char* key, const wchar_t* value);
	void add(const char* key, const wxString& value);
	void add(const char* key, AtObj& data);
	void set(const char* key, const wchar_t* value);
	void set(const char* key, const wxString& value);
	void set(const char* key, AtObj& data);
	void setBool(const char* key, bool value);
	void setDouble(const char* key, double value);
	void setInt(const char* key, int value);
	void setString(const wchar_t* value);
	void addOverlay(AtObj& data);

	AtSmartPtr<const AtNode> m_Node;
};


// Miscellaneous utility functions:
namespace AtlasObject
{
	// Returns AtObj() on failure - test with AtObj::defined()
	AtObj LoadFromXML(const std::string& xml);

	// Returns AtObj() on failure - test with AtObj::defined()
	AtObj LoadFromJSON(const std::string& json);

	// Returns UTF-8-encoded XML document string.
	// Returns empty string on failure.
	std::string SaveToXML(AtObj& obj);

	// Returns UTF-8-encoded JSON string.
	// Returns empty string on failure.
	std::string SaveToJSON(AtObj& obj);

	AtObj TrimEmptyChildren(AtObj& obj);
}

#endif // INCLUDED_ATLASOBJECT
