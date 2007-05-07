#ifndef INCLUDED_DELTAARRAY
#define INCLUDED_DELTAARRAY

template<typename T> class DeltaArray2D
{
public:
	virtual ~DeltaArray2D() {}

	T get(int x, int y);
	void set(int x, int y, const T& val);

	void OverlayWith(const DeltaArray2D<T>& overlayer);
	void Undo();
	void Redo();

protected:
	virtual T getOld(int x, int y) = 0;
	virtual void setNew(int x, int y, const T& val) = 0;

private:
	struct hashfunc {
		enum {
			bucket_size = 4,
			min_buckets = 8
		};
		size_t operator()(const std::pair<int, int>& p) const {
			return STL_HASH_VALUE(p.first << 16) + STL_HASH_VALUE(p.second);
		}
		bool operator()(const std::pair<int, int>& a, const std::pair<int, int>& b) const {
			return (a < b);
		}
	};
	// TODO: more efficient representation
	typedef STL_HASH_MAP<std::pair<int, int>, std::pair<T, T>, hashfunc> Data; // map of <x,y> -> <old_val, new_val>
	Data m_Data;
};

//////////////////////////////////////////////////////////////////////////

template<typename T>
T DeltaArray2D<T>::get(int x, int y)
{
	typename Data::iterator it = m_Data.find(std::make_pair(x, y));
	if (it == m_Data.end())
		return getOld(x, y);
	else
		return it->second.second;
}

template<typename T>
void DeltaArray2D<T>::set(int x, int y, const T& val)
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
