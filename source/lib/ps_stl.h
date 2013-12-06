/* Copyright (c) 2013 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef INCLUDED_PS_STL
#define INCLUDED_PS_STL

/**
 * @author Jorma Rebane
 * @note Pyrogenesis STL methods
 * @note This file contains useful and optimized methods for use with STL
 */

namespace ps 
{
	/**
	 * Removes the first occurrence of the specified value from the container.
	 * @param container The STL-compatible container to remove from.
	 * @param value The value to remove.
	 */
template<class Container, class T>
	inline void remove_first_occurrence(Container& container, const T& value)
	{
		if (int count = container.size())
		{
			T* data = &container[0];
			for (int i = 0; i < count; ++i)
			{
				if (data[i] == value)
				{
					container.erase(container.begin() + i);
					return;
				}
			}
		}
	}

	/**
	 * @param container The STL-compatible container to search in.
	 * @param value The value to search for.
	 * @return TRUE if [value] exists in [container].
	 */
template<class Container, class T>
	inline bool exists_in(const Container& container, const T& value)
	{
		if (int count = container.size())
		{
			for (const T* data = &container[0]; count; ++data, --count)
			{
				if (*data == value)
				{
					return true;
				}
			}
		}
		return false;
	}

	/**
	 * Finds a value in a container
	 * @param container The STL-compatible container to search in.
	 * @param value The value to search for.
	 * @return Pointer to the value if found, NULL if not found.
	 */
template<class Container, class T>
	inline T* find_in(const Container& container, const T& value)
	{
		if (int count = container.size())
		{
			for (const T* data = &container[0]; count; ++data, --count)
			{
				if (*data == value)
				{
					return (T*)data;
				}
			}
		}
		return NULL;
	}


} // namespace ps

#endif // INCLUDED_PS_STL