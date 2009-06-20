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

#ifndef INCLUDED_DELTAARRAY
#define INCLUDED_DELTAARRAY

template<typename T> class DeltaArray2D
{
public:
	virtual ~DeltaArray2D() {}

	T get(ssize_t x, ssize_t y);
	void set(ssize_t x, ssize_t y, const T& val);

	void OverlayWith(const DeltaArray2D<T>& overlayer);
	void Undo();
	void Redo();

protected:
	virtual T getOld(ssize_t x, ssize_t y) = 0;
	virtual void setNew(ssize_t x, ssize_t y, const T& val) = 0;

private:
	struct hashfunc {
		enum {
			bucket_size = 4,
			min_buckets = 8
		};
		size_t operator()(const std::pair<ssize_t, ssize_t>& p) const {
			return STL_HASH_VALUE(p.first << 16) + STL_HASH_VALUE(p.second);
		}
		bool operator()(const std::pair<ssize_t, ssize_t>& a, const std::pair<ssize_t, ssize_t>& b) const {
			return (a < b);
		}
	};
	// TODO: more efficient representation
	typedef STL_HASH_MAP<std::pair<ssize_t, ssize_t>, std::pair<T, T>, hashfunc> Data; // map of <x,y> -> <old_val, new_val>
	Data m_Data;
};

//////////////////////////////////////////////////////////////////////////

template<typename T>
T DeltaArray2D<T>::get(ssize_t x, ssize_t y)
{
	typename Data::iterator it = m_Data.find(std::make_pair(x, y));
	if (it == m_Data.end())
		return getOld(x, y);
	else
		return it->second.second;
}

template<typename T>
void DeltaArray2D<T>::set(ssize_t x, ssize_t y, const T& val)
{
	typename Data::iterator it = m_Data.find(std::make_pair(x, y));
	if (it == m_Data.end())
		m_Data.insert(std::make_pair(std::make_pair(x, y), std::make_pair(getOld(x, y), val)));
	else
		it->second.second = val;
	setNew(x, y, val);
}

template <typename T>
void DeltaArray2D<T>::OverlayWith(const DeltaArray2D<T>& overlayer)
{
	for (typename Data::const_iterator it = overlayer.m_Data.begin(); it != overlayer.m_Data.end(); ++it)
	{
		typename Data::iterator it2 = m_Data.find(it->first);
		if (it2 == m_Data.end())
			m_Data.insert(*it);
		else
		{
			//debug_assert(it2->second.second == it->second.first);
			it2->second.second = it->second.second;
		}
	}

}

template <typename T>
void DeltaArray2D<T>::Undo()
{
	for (typename Data::iterator it = m_Data.begin(); it != m_Data.end(); ++it)
		setNew(it->first.first, it->first.second, it->second.first);
}

template <typename T>
void DeltaArray2D<T>::Redo()
{
	for (typename Data::iterator it = m_Data.begin(); it != m_Data.end(); ++it)
		setNew(it->first.first, it->first.second, it->second.second);
}

#endif // INCLUDED_DELTAARRAY
