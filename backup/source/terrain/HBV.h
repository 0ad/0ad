// HBV.h
//
// (c) Rich Cross, 2000


#ifndef __HBV_H
#define __HBV_H

#include <algorithm>
#include <vector>
#include "HBVNode.h"
#include "Aggregate.h"

class Point3;
class Vector3;

template <class T>
class HBV 
{
public:
    HBV();
    ~HBV();

	Geometry* clone() const;

    void open();
    void add(const T& element);
    void addMany(const std::vector<T>& v);
    void close();

    bool vectorIntersect(const Point3& origin,const Vector3& dir,float& dist) const;
    void getBounds(BoundingBox& result) const;
    void getNormal(const Point3& pt,Vector3& result) const;
    void getUV(const Point3& pt,float& u,float& v) const;

	const T& getIntersectedElement() const;

private:
    mutable T _cachedElement;
    HBVNode<T>* _root;
	std::vector<T>* _elementList;
};


#include "common.h"
#include "HBVLeaf.h"



template <class T>
HBV<T>::HBV() : _root(0), _elementList(0), _cachedElement(0)
{
}

template <class T>
HBV<T>::~HBV()
{
	assert(_elementList==0);
	delete _root;
}

template <class T>
Geometry* HBV<T>::clone() const
{
	assert(0);
	return 0;
}

template <class T>
void HBV<T>::open()
{
    _elementList=new std::vector<T>();
}

template <class T>
void HBV<T>::add(const T& element)
{
    assert(_elementList!=0);
    _elementList->push_back(element);
}

template <class T>
void HBV<T>::addMany(const std::vector<T>& v)
{
	assert(_elementList!=0);
	for (int i=0;i<v.size();i++) 
		_elementList->push_back(v.at(i));
}

template <class T>
void HBV<T>::close()
{
	std::random_shuffle(_elementList->begin(),_elementList->end());

    for (int i=0;i<_elementList->size();i++) {
        T element=_elementList->at(i);
        HBVLeaf<T> *leaf=new HBVLeaf<T>(element);
        if (_root) {
            _root->addLeaf(leaf);
			while (_root->getParent())
				_root=_root->getParent();
		}
        else
            _root=leaf;
    }

    _elementList->clear();
    delete _elementList;
	_elementList=0;
}

template <class T>
bool HBV<T>::vectorIntersect(const Point3& origin,const Vector3& dir,float& dist) const
{
    T element;

    _cachedElement=0;

    int result=_root->vectorIntersect(origin,dir,dist,element);
    if (result) {
        _cachedElement=element;
        return true;
    }

    return false;
}

template <class T>
void HBV<T>::getBounds(BoundingBox& result) const
{
    result=_root->getBounds();
}


template <class T>
void HBV<T>::getNormal(const Point3& pt,Vector3& result) const
{
    assert(_cachedElement!=0);
    _cachedElement->getNormal(pt,result);
}

template <class T>
void HBV<T>::getUV(const Point3& pt,float& u,float& v) const
{
    assert(_cachedElement!=0);
    _cachedElement->getUV(pt,u,v);
}

template <class T>
const T& HBV<T>::getIntersectedElement() const
{
    assert(_cachedElement!=0);
	return _cachedElement;
}

#endif