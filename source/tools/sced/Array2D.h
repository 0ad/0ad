#ifndef _ARRAY2D_H
#define _ARRAY2D_H

template <class T> 
class CArray2D 
{
public:
	CArray2D();
	CArray2D(int usize,int vsize);
	CArray2D(int usize,int vsize,const T* data);
	CArray2D(const CArray2D& rhs);
	virtual ~CArray2D();

	CArray2D& operator=(const CArray2D& rhs);
	
	operator T*();
	operator const T*() const;

	int usize() const;
	int vsize() const;
	void size(int& usize,int& vsize) const;

	void resize(int usize,int vsize);

	void fill(const T* elements);

	T& operator()(int a,int b);
  	const T& operator()(int a,int b) const;

	T& at(int a,int b);
  	const T& at(int a,int b) const;

protected:
	// allocate and deallocate are overrideable by subclasses, allowing alternative
	// (more efficient) allocation in certain circumstances 
	//	* if allocation/deallocation is not done using new[]/delete[], it is necessary
	// to override both functions
	virtual void allocate();	
	virtual void deallocate();

	int _usize;			// no of columns; 
	int _vsize;			// no of rows
	T* _elements;
};

///////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <string.h>

template <class T>
CArray2D<T>::CArray2D() : _usize(0), _vsize(0), _elements(0)
{
}

template <class T>
CArray2D<T>::CArray2D(int usize,int vsize) 
	: _usize(usize), _vsize(vsize), _elements(0)
{
	allocate();
}

template <class T>
CArray2D<T>::CArray2D(int usize,int vsize,const T* elements) 
	: _usize(usize), _vsize(vsize), _elements(0)
{
	assert(elements!=0);

	allocate();
	fill(elements);
}

template <class T>
CArray2D<T>::CArray2D(const CArray2D<T>& rhs) 
	: _usize(rhs._usize), _vsize(rhs._vsize), _elements(0)
{
	allocate();
	fill(rhs._elements);
}

template <class T>
CArray2D<T>::~CArray2D()
{
	deallocate();
}

template <class T>
CArray2D<T>& CArray2D<T>::operator=(const CArray2D<T>& rhs)
{
	if (this==&rhs)
		return *this;

	deallocate();

	_usize=rhs._usize;
	_vsize=rhs._vsize;
	
	allocate();
	fill(rhs._elements);

	return *this;
}

template <class T>
void CArray2D<T>::allocate()
{
	assert(_elements==0);
	_elements=new T[_usize*_vsize];
}

template <class T>
void CArray2D<T>::deallocate()
{
	delete[] _elements;
	_elements=0;
}

template <class T>
int CArray2D<T>::usize() const
{
	return _usize;
}

template <class T>
int CArray2D<T>::vsize() const
{
	return _vsize;
}

template <class T>
void CArray2D<T>::size(int& usize,int& vsize) const
{
	usize=_usize;
	vsize=_vsize;
}

template <class T>
void CArray2D<T>::resize(int usize,int vsize) 
{
	deallocate();

	_usize=usize;
	_vsize=vsize;
	allocate();
}

template <class T>
void CArray2D<T>::fill(const T* elements)
{
	memcpy(_elements,elements,sizeof(T)*_usize*_vsize);
}


// operator()(int r,int c)
//		return the element at (r,c) (ie c'th element in r'th row) 
template <class T>
T& CArray2D<T>::operator()(int u,int v)
{
	assert(u>=0 && u<_usize);
	assert(v>=0 && v<_vsize);
	return _elements[(u*_vsize)+v];
}

template <class T>
const T& CArray2D<T>::operator()(int u,int v) const
{
	assert(u>=0 && u<_usize);
	assert(v>=0 && v<_vsize);
	return _elements[(u*_vsize)+v];
}

template <class T>
T& CArray2D<T>::at(int u,int v)
{
	assert(u>=0 && u<_usize);
	assert(v>=0 && v<_vsize);
	return _elements[(u*_vsize)+v];
}

template <class T>
const T& CArray2D<T>::at(int u,int v) const
{
	assert(u>=0 && u<_usize);
	assert(v>=0 && v<_vsize);
	return _elements[(u*_vsize)+v];
}

template <class T>
CArray2D<T>::operator T*()
{
	return _elements;
}

template <class T>
CArray2D<T>::operator const T*() const
{
	return _elements;
}

///////////////////////////////////////////////////////////////////////////

#endif
