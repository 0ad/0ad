/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FMArray.h
	The file contains the vector class, which improves on the standard C++ vector class.
 */

/**
	A dynamically-sized array.
	Built on top of the standard C++ vector class, this class improves on the interface
	by adding useful extra functionality, such as constructor that takes in a constant-sized array,
	comparison which a constant-sized array, erase and find functions that take in a value, etc.

	@ingroup FMath
*/

#ifndef _FM_ARRAY_H_
#define _FM_ARRAY_H_

#pragma warning(push)
#pragma warning(disable : 4275) // Harmless warning due to exported fm::vector class deriving from non-exported std::vector.

/** Namespace that contains the overwritten STL classes. */
namespace fm
{
	/** A STL vector with extra shortcuts to make the code more readable.
		The most important shortcuts are 'contains', 'find' and 'erase'. */
	template <class T>
	class vector : public std::vector<T>
	{
	private:
		typedef typename std::vector<T> Parent;

	public:
		/** The basic list iterator. */
		typedef typename Parent::iterator iterator;

		/** The non-modifiable list iterator. */
		typedef typename Parent::const_iterator const_iterator;

	public:
		/** Default constructor. */
		vector() : Parent() {}

		/** Constructor. Builds a dynamically-sized array of the wanted size.
			@param size The wanted size of the array. */
		vector(size_t size) : Parent(size) {}

		/** Constructor. Builds a dynamically-sized array of the wanted size.
			@param size The wanted size of the array
			@param defaultValue The default value to use for all the entries of the array. */
		vector(size_t size, const T& defaultValue) : Parent(size, defaultValue) {}

		/** Copy constructor.
			@param copy The dynamically-sized array to copy the values from. */
		vector(const std::vector<T>& copy) : Parent(copy) {}

		/** Constructor. Builds a dynamically-sized array from a constant-sized array.
			@param values A constant-sized array of floating-point values.
			@param count The size of the constant-sized array. */
		vector(const T* values, size_t count) : Parent()
		{
			resize(count);
			memcpy(&Parent::at(0), values, count * sizeof(T));
		}

		/** Retrieves an iterator for a given element.
			@param value The value, contained within the list, to search for.
			@return An iterator to this element. The end() iterator
				is returned if the value is not found. */
		template <typename ValueType>
		inline iterator find(const ValueType& value) { return std::find(begin(), end(), value); }
		template <typename ValueType>
		inline const_iterator find(const ValueType& value) const { return std::find(begin(), end(), value); } /**< See above. */

		/** Removes the value at the given position within the list.
			@param it The list position for the value to remove. */
		inline iterator erase(iterator it) { return Parent::erase(it); }

		/** Removes a range of values from the list. The range is determined as every value
			between and including the first value, up to the last value, but not including the
			last value.
			@param first The first list value to remove.
			@param last The last list value. This value will be kept. */
		inline void erase(iterator first, iterator last) { Parent::erase(first, last); }

		/** Removes a value contained within the list, once.
			@param value The value, contained within the list, to erase from it.
			@return Whether the value was found and erased from the list. */
		inline bool erase(const T& value) { iterator it = find(value); if (it != end()) { erase(it); return true; } return false; }

		/** Removes an indexed value contained within the list.
			@param index The index of the value to erase. */
		inline void erase(size_t index) { erase(begin() + index); }

		/** Releases a value contained within a list.
			Use this function only if your vector contains pointers
			and you are certain that there is no duplicate pointers within the list.
			@param value The value, contained within the list, to release.
			@return Whether the value was found and released. */
		inline bool release(const T& value) { iterator it = find(value); if (it != end()) { erase(it); delete value; return true; } return false; }

		/** Retrieves whether the list contains a given value.
			@param value A value that could be contained in the list.
			@return Whether the list contains this value. */
		inline bool contains(const T& value) const { const_iterator it = find(value); return it != end(); }

		/** Retrieves the number of values contained in the list.
			@return The number of values contained in the list. */
		inline size_t size() const { return Parent::size(); }

		/** Sets the number of values contained in the list.
			@param count The new number of values contained in the list.
			@param default The value to assign to the new entries in the list. */
		inline void resize(size_t count, const T& value = T()) { Parent::resize(count, value); }

		/** Pre-allocate the list to a certain number of values.
			You can use reserve zero values in order to clear the memory
			used by this list. This function is useful when optimizing.
			@param count The new number of values pre-allocated in the list. */
		inline void reserve(size_t count) { Parent::reserve(count); }

		/** Retrieves the iterator for the first value in the list.
			@return The iterator for the first value in the list. */
		inline iterator begin() { return Parent::begin(); }
		inline const_iterator begin() const { return Parent::begin(); } /**< See above. */

		/** Retrieves the iterator for the last value in the list.
			@return The iterator for the last value in the list. */
		inline iterator end() { return Parent::end(); }
		inline const_iterator end() const { return Parent::end(); } /**< See above. */
	};

	/** A STL string. */
	typedef std::string string;

	/** A STL pair */
	template <typename _Kty, typename _Ty>
	class pair : public std::pair<_Kty, _Ty> {};

	/** A STL map. */
	template <typename _Kty, typename _Ty>
	class map : public std::map<_Kty, _Ty> {};

	/** A STL MultiMap */
	template <typename _Kty, typename _Ty>
	class multimap : public std::multimap<_Kty, _Ty> {};

	/* STL list */
	template <typename _Ty>
	class list : public std::list<_Ty> {};
};

/** Returns whether a dynamically-sized array is equivalent to a constant-sized array.
	@param dl A dynamically-sized array.
	@param cl A constant-sized array.
	@param count The size of the constant-sized array.
	@return Whether the two arrays are equivalent. */
template <typename T>
inline bool IsEquivalent(const std::vector<T>& dl, const T* cl, size_t count)
{
	if (dl.size() != count) return false;
	bool equivalent = true;
	for (size_t i = 0; i < count && equivalent; ++i){
		 equivalent = IsEquivalent(dl.at(i), cl[i]);
	}
	return equivalent;
}

#pragma warning(pop)

#endif // _FM_ARRAY_H_
