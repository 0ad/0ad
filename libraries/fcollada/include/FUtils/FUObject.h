/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	The idea is simple: the object registers itself in the constructor, with one container.
	It may register/unregister itself with other containers subsequently.

	It keeps a list of the containers and a pointer to the main container so that the up-classes
	can easily access it. In the destructor, the object unregisters itself with the container(s).

	The object has a RTTI-like static class structure so that the container(s) know which
	object type list to remove this object from when it is deleted.
*/

/**
	@file FUObject.h
	This file contains the FUObject class, the FUObjectContainer class and the FUObjectType class.
*/

#ifndef _FU_OBJECT_H_
#define _FU_OBJECT_H_

#ifndef _FU_OBJECT_TYPE_H_
#include "FUtils/FUObjectType.h"
#endif // _FU_OBJECT_TYPE_H_

class FUObjectTracker;
typedef fm::pvector<FUObjectTracker> FUObjectTrackerList; /**< A dynamically-sized array of object trackers. */

/**
	A basic object.

	Each object holds a pointer to the trackers that track it.
	This pointer is useful so that the trackers can be notified if the object
	is released.

	Each up-class of this basic object class hold an object type
	that acts just like RTTI to provide a safe way to up-cast.

	@ingroup FUtils
*/
class FCOLLADA_EXPORT FUObject
{
private:
	static class FUObjectType* baseObjectType;
	FUObjectTrackerList trackers;

public:
	/** Constructor.
		Although it is not an abstract class, this class is
		not meant to be used directly.  */
	FUObject();

	/** Destructor.
		This function informs the object lists of this object's release. */
	virtual ~FUObject();

	/** Releases this object.
		This function essentially calls the destructor.
		This function is virtual and is always
		overwritten when using the ImplementObjectType macro. */
	virtual void Release();

	/** Retrieves the type of the base object class.
		@return The type of the base object class. */
	static const FUObjectType& GetClassType() { return *baseObjectType; }

	/** Retrieves the type of the object class.
		@return The type of the base object class. */
	virtual const FUObjectType& GetObjectType() const { return *baseObjectType; }

	/** Retrieves whether this object has exactly the given type.
		@param _type A given class type.
		@return Whether this object is exactly of the given type. */
	inline bool IsType(const FUObjectType& _type) const { return GetObjectType() == _type; }

	/** Retrieves whether this object is exactly or inherits the given type.
		@param _type A given class type.
		@return Whether this object is exactly or inherits the given type. */
	inline bool HasType(const FUObjectType& _type) const { return GetObjectType().Includes(_type); }

	/** Retrieves the number of tracker tracking the object.
		This can be used as an expensive reference counting mechanism.
		@return The number of trackers tracking the object. */
	size_t GetTrackerCount() const { return trackers.size(); }

	/** Retrieves a reference to the wanted tracker.
		@param idx The tracker index.
		@return The tracker, or NULL if idx is not smaller than the number of trackers. */
	const FUObjectTracker *GetTracker( size_t idx ) const { FUAssert( idx < trackers.size(), return NULL ); return trackers[idx]; }

protected:
	/** Detaches all the trackers of this object.
		The trackers will be notified that this object has been released.
		It is not recommended to call this function outside of a destructor. */
	void Detach();

private:
	friend class FUObjectTracker;
	void AddTracker(FUObjectTracker* tracker);
	void RemoveTracker(FUObjectTracker* tracker);
	bool HasTracker(const FUObjectTracker* tracker) const;
};

/**
	An object set
	Each set has access to a list of unique objects.
	When the objects are created/released: they will inform the
	list.
	@ingroup FUtils
*/
class FCOLLADA_EXPORT FUObjectTracker
{
public:
	/** Destructor. */
	virtual ~FUObjectTracker() {}

	/** Callback when an object tracked by this tracker
		is being released.
		@param object A tracked object. */
	virtual void OnObjectReleased(FUObject* object) = 0;

	/** Retrieves whether an object is tracked by this tracker.
		@param object An object. */
	virtual bool TracksObject(const FUObject* object) const { return object != NULL ? object->HasTracker(this) : false; }

protected:
	/** Adds an object to be tracked.
		@param object The object to track. */
	void TrackObject(FUObject* object) { if(object) object->AddTracker(this); }

	/** Stops tracking an object
		@param object The object to stop tracking. */
	void UntrackObject(FUObject* object) { if(object) object->RemoveTracker(this); }
};

/**
	A tracked object pointer
	The reverse idea of a smart pointer: if the object pointed
	to by the pointer is released, the pointer will become NULL.
	@ingroup FUtils
*/
template <class ObjectClass = FUObject>
class FUObjectPtr : public FUObjectTracker
{
protected:
	/** The tracked pointer. */
	ObjectClass* ptr;

public:
	/** Copy constructor.
		@param _ptr The object to track. This pointer can be NULL to indicate
			that no object should be tracked at this time. */
	FUObjectPtr(ObjectClass* _ptr = NULL) : ptr(_ptr)
	{
		if (ptr != NULL) FUObjectTracker::TrackObject((FUObject*) ptr);
		ptr = ptr;
	}

	/** Destructor.
		Stops the tracking of the pointer. */
	~FUObjectPtr()
	{
		if (ptr != NULL) FUObjectTracker::UntrackObject((FUObject*) ptr);
		ptr = NULL;
	}

	/** Assigns this tracking pointer a new object to track.
		@param _ptr The new object to track.
		@return This reference. */
	FUObjectPtr& operator=(ObjectClass* _ptr)
	{
		if (ptr != NULL) FUObjectTracker::UntrackObject((FUObject*) ptr);
		ptr = _ptr;
		if (ptr != NULL) FUObjectTracker::TrackObject((FUObject*) ptr);
		return *this;
	}
	inline FUObjectPtr& operator=(const FUObjectPtr& _ptr) { return operator=(_ptr.ptr); } /**< See above. */

	/** Retrieves whether an object is tracked by this tracker.
		@param object An object. */
	virtual bool TracksObject(const FUObject* object) const { return (FUObject*) ptr == object; }

	/** Accesses the tracked object.
		@return The tracked object. */
	inline ObjectClass& operator*() { FUAssert(ptr != NULL, return *ptr); return *ptr; }
	inline const ObjectClass& operator*() const { FUAssert(ptr != NULL, return *ptr); return *ptr; } /**< See above. */
	inline ObjectClass* operator->() { return ptr; } /**< See above. */
	inline const ObjectClass* operator->() const { return ptr; } /**< See above. */
	inline operator ObjectClass*() { return ptr; } /**< See above. */
	inline operator const ObjectClass*() const { return ptr; } /**< See above. */

protected:
	/** Callback when an object tracked by this tracker
		is being released.
		@param object A contained object. */
	virtual void OnObjectReleased(FUObject* object)
	{
		FUAssert(TracksObject(object), return);
		ptr = NULL;
	}
};

/**
	An object reference
	On top of the tracked object pointer, when this reference
	is released: the tracked object is released.

	This is template is very complex for a reference.
	You get reduced compilation times and support for
	multiple containment references.
	
	@ingroup FUtils
*/
template <class ObjectClass = FUObject>
class FUObjectRef : public FUObjectPtr<ObjectClass>
{
private:
	typedef FUObjectPtr<ObjectClass> Parent;

public:
	/** Copy constructor.
		@param ptr The object to reference. This pointer can be NULL to indicate
			that no object should be referenced at this time. */
	FUObjectRef(ObjectClass* _ptr = NULL)
	:	FUObjectPtr<ObjectClass>(_ptr)
	{
	}

	/** Destructor.
		The object referenced will be released. */
	~FUObjectRef()
	{
		FUObject* _ptr = (FUObject*) Parent::operator->();
		SAFE_RELEASE(_ptr);
	}

	/** Sets a new object to reference.
		The previously tracked object will be released.
		@param _ptr The new object to reference.
		@return This reference. */
	FUObjectRef& operator=(ObjectClass* __ptr)
	{
		FUObject* _ptr = (FUObject*) Parent::operator->();
		SAFE_RELEASE( _ptr);
		Parent::operator=(__ptr);
		return *this;
	}
	inline FUObjectRef& operator=(FUObjectPtr<ObjectClass>& _ptr) { return operator=(_ptr.ptr); } /**< See above. */
	inline FUObjectRef& operator=(FUObjectRef& _ptr) { return operator=(_ptr.ptr); } /**< See above. */
};

/**
	An object list.
	Based on top of our modified version of the STL vector class,
	this contained object list holds pointers to some FUObject derived class
	and automatically removes objects when they are deleted.
	@ingroup FUtils
*/
template <class ObjectClass = FUObject>
class FUObjectList : public fm::pvector<ObjectClass>, FUObjectTracker
{
public:
	typedef fm::pvector<ObjectClass> Parent;
	typedef ObjectClass* item;
	typedef const ObjectClass* const_item;
	typedef item* iterator;
	typedef const_item* const_iterator;

	/** Destructor. */
	virtual ~FUObjectList() { clear(); }

	/** Clears the object tracked by this object list. */
	void clear()
	{
		for (iterator it = begin(); it != end(); ++it)
		{
			FUObjectTracker::UntrackObject((FUObject*) (*it));
		}
		Parent::clear();
	}
	
	/** Retrieves the first element of the container.
		@return The first element in the container. */
	ObjectClass*& front() { return (ObjectClass*&) Parent::front(); }
	const ObjectClass*& front() const { return (const ObjectClass*&) Parent::front(); } /**< See above. */

	/** Retrieves the last element of the container.
		@return The last element in the container. */
	ObjectClass*& back() { return (ObjectClass*&) Parent::back(); }
	const ObjectClass*& back() const { return (const ObjectClass*&) Parent::back(); } /**< See above. */

	/** Retrieves an indexed object in the list.
		@param index An index.
		@return The given object. */
	inline ObjectClass* at(size_t index) { return (ObjectClass*) Parent::at(index); }
	inline const ObjectClass* at(size_t index) const { return (ObjectClass*) Parent::at(index); } /**< See above. */
	template <class INTEGER> inline ObjectClass* operator[](INTEGER index) { return at(index); } /**< See above. */
	template <class INTEGER> inline const ObjectClass* operator[](INTEGER index) const { return at(index); } /**< See above. */

	/** Retrieves an iterator for the first element in the list.
		@return an iterator for the first element in the list. */
	inline iterator begin() { return (!Parent::empty()) ? &front() : NULL; }
	inline const_iterator begin() const { return (!Parent::empty()) ? &front() : NULL; } /**< See above. */
	
	/** Retrieves an iterator for the element after the last element in the list.
		@return an iterator for the element after the last element in the list. */
	inline iterator end() { return (!Parent::empty()) ? (&back()) + 1 : NULL; }
	inline const_iterator end() const { return (!Parent::empty()) ? (&back()) + 1 : NULL; } /**< See above. */
	
	/** Retrieves an iterator for a given element in the list.
		@param item An item of the list.
		@return An iterator for the given item. If the item is not
			found in the list, the end() iterator is returned. */
	inline iterator find(const ObjectClass* item) { iterator f = Parent::find(item); return begin() + (f - begin()); }
	inline const_iterator find(const ObjectClass* item) const { const_iterator f = Parent::find(item); return begin() + (f - begin()); }

	/** Adds an object to the container's containment list.
		@param object An object to contain. */
	inline void push_back(ObjectClass* object)
	{
		FUObjectTracker::TrackObject((FUObject*)object);
		Parent::push_back(object);
	}

	/** Inserts an object in the container's containment list.
		@param _iterator The iterator after which to insert the object.
		@param object An object to insert.
		@return The iterator to the inserted object. */
	inline iterator insert(iterator _iterator, ObjectClass* object)
	{
		FUObjectTracker::TrackObject(object);
		iterator originalStart = begin();
		iterator newIt = Parent::insert(Parent::begin() + (_iterator - originalStart), object);
		return begin() + (newIt - Parent::begin());
	}

	/** Inserts a list of object in the container's containment list.
		@param _where The iterator after which to insert the object.
		@param startIterator The iterator for the first object to insert.
		@param endIterator The iterator for the last object.
			This object will not be inserted. */
	template <class _It>
	inline void insert(iterator _where, _It _startIterator, _It _endIterator)
	{
		size_t relativeWhere = _where - begin();
		size_t count = _endIterator - _startIterator;
		Parent::insert(Parent::begin() + relativeWhere, count);
		_where = begin() + relativeWhere;

		for (; _startIterator != _endIterator; ++_startIterator, ++_where)
		{
			*_where = const_cast<ObjectClass*>((const ObjectClass*)(*_startIterator));
			FUObjectTracker::TrackObject(const_cast<FUObject*>((const FUObject*) (*_startIterator)));
		}
	}

	/** Removes the last value of the tracked object list. */
	inline void pop_back()
	{
		if (!Parent::empty())
		{
			FUObjectTracker::UntrackObject(back());
			Parent::pop_back();
		}
	}
	
	/** Removes the value at the given position within the list.
		@param it The list position for the value to remove. */
	inline iterator erase(iterator _it)
	{
		FUObjectTracker::UntrackObject((FUObject*) *_it);
		iterator it = Parent::begin() + (_it - begin());
		it = Parent::erase(it);
		return begin() + (it - Parent::begin());
	}

	/** Removes the value at the given position within the list.
		@param it The list position for the value to remove. */
	inline void erase(iterator first, iterator last)
	{
		for (iterator it = first; it != last; ++it) FUObjectTracker::UntrackObject((FUObject*) *it);
		Parent::erase(first, last);
	}

	/** Removes a value contained within the list, once.
		@param value The value, contained within the list, to erase from it.
		@return Whether the value was found and erased from the list. */
	inline bool erase(const ObjectClass* value)
	{
		iterator it = Parent::find(value);
		if (it != Parent::end())
		{
			FUObjectTracker::UntrackObject((FUObject*) *it);
			Parent::erase(it);
			return true;
		}
		return false;
	}

	/** Removes an indexed value contained within the list.
		@param index The index of the value to erase. */
	inline void erase(size_t index) { erase(begin() + index); }

	/** Releases a value contained within a list.
		Use this function only if your vector contains pointers
		and you are certain that there is no duplicate pointers within the list.
		@param value The value, contained within the list, to release.
		@return Whether the value was found and released. */
	inline bool release(const ObjectClass* value)
	{
		iterator it = Parent::find(value);
		if (it != Parent::end()) { Parent::erase(it); ((FUObject*) value)->Release(); return true; }
		return false;
	}

	/** Retrieves whether an object is contained by this container.
		@param object An object. */
	virtual bool TracksObject(const FUObject* object) const { return Parent::contains((ObjectClass*) object); }

	/** @todo Write a nice description. */
	FUObjectList<ObjectClass>& operator= (const FUObjectList<ObjectClass>& other) { clear(); insert(end(), other.begin(), other.end()); return *this; }

protected:
	/** Removes an object from the container's containment list.
		@param object A contained object. */
	virtual void OnObjectReleased(FUObject* object)
	{
		FUAssert(TracksObject(object), return);
		Parent::erase((ObjectClass*) object);
	}
};

/**
	A contained object list.
	When this list is released, the contained objects are also released.
	In theory, each object should be contained only once, but may be tracked multiple times.
	@ingroup FUtils
*/
template <typename ObjectClass = FUObject>
class FUObjectContainer : public FUObjectList<ObjectClass>
{
private:
	typedef FUObjectList<ObjectClass> Parent;
	
public:
	/** Destructor.
		Releases all the objects contained within this container. */
	virtual ~FUObjectContainer() { clear(); }

	/** Clears and releases the object tracked by this object list. */
	void clear()
	{
#ifdef _DEBUG
		size_t arraySize=size();
#endif
		while (size() > 0)
		{
			((FUObject*) Parent::back())->Release();
#ifdef _DEBUG
			if(size() >= arraySize)
				FUFail(return); //if you crash here, you're probably deleting something you shouldn't be, or not cloning correctly an object.
			arraySize = size();
#endif
		}
	}
	
	/** Retrieves the number of elements in the container.
		@return The number of elements in the container. */
	size_t size() const { return Parent::size(); }
	
	/** Adds a new empty object to the container.
		@return The new empty object. */
	ObjectClass* Add()
	{
		ObjectClass* object = new ObjectClass();
		push_back(object);
		return object;
	}

	/** Adds a new object to the container.
		@param arg1 An constructor argument.
		@return The new object. */
	template <class A1>
	ObjectClass* Add(const A1& arg1)
	{
		ObjectClass* object = new ObjectClass(arg1);
		push_back(object);
		return object;
	}

	/** Adds a new object to the container.
		@param arg1 A first constructor argument.
		@param arg2 A second constructor argument.
		@return The new object. */
	template <class A1, class A2>
	ObjectClass* Add(const A1& arg1, const A2& arg2)
	{
		ObjectClass* object = new ObjectClass(arg1, arg2);
		push_back(object);
		return object;
	}

	/** Adds a new object to the container.
		@param arg1 A first constructor argument.
		@param arg2 A second constructor argument.
		@param arg3 A third constructor argument.
		@return The new object. */
	template <class A1, class A2, class A3>
	ObjectClass* Add(const A1& arg1, const A2& arg2, const A3& arg3)
	{
		ObjectClass* object = new ObjectClass(arg1, arg2, arg3);
		push_back(object);
		return object;
	}
};

/**
	Macros used to dynamically case a given FUObject pointer to some higher
	class object.
*/
template <class HigherClassType>
inline HigherClassType* DynamicCast(FUObject* object) { return object->HasType(HigherClassType::GetClassType()) ? (HigherClassType*) object : NULL; }

#endif // _FU_OBJECT_H_
