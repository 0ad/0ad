#ifndef _AABBTREE_H
#define _AABBTREE_H

#include <vector>
#include <algorithm>
#include "Bound.h"

template <class T>
class CAABBTree 
{
private:
	struct Leaf;
	struct Branch;
	struct InsertionData;

	struct Node {
		Node() : m_Parent(0) {}
		virtual ~Node() {}

		virtual bool RayIntersect(CVector3D& origin,CVector3D& dir,float& dist,T* selection) = 0;
		virtual void FindInsertionPoint(Leaf* _leaf,InsertionData& _inserter) = 0;

		virtual void AddLeaf(Leaf* _leaf) = 0;
		virtual void AddLeafAsChild(Leaf* _leaf) {};
		void AddLeafAsSibling(Leaf* _leaf);

		CBound m_Bounds;
		Branch* m_Parent;
	};

	struct Leaf : public Node {		
		
		T m_Element;
		void AddLeaf(Leaf* _leaf);
		bool RayIntersect(CVector3D& origin,CVector3D& dir,float& dist,T* selection);
		void FindInsertionPoint(Leaf* _leaf,InsertionData& _inserter);
	};

	struct Branch : public Node {

		~Branch() {
			for (int i=0;i<m_Children.size();i++) {
				delete m_Children[i];
			}
		}

		void AddChild(Node* _node);
		void RemoveChild(Node* _node);

		void AddLeaf(Leaf* _leaf);
		void AddLeafAsChild(Leaf* leaf);

		bool RayIntersect(CVector3D& origin,CVector3D& dir,float& dist,T* selection);
		void FindInsertionPoint(Leaf* _leaf,InsertionData& _inserter);

		std::vector<Node*> m_Children;
	};

	struct InsertionData
	{
		enum Method { ADDINVALID, ADDASSIBLING, ADDASCHILD };
	
		InsertionData() : cost((float) 1.0e38), inheritedCost(0), method(ADDINVALID), insertionPt(0) {}
		void setInsertion(Node* pt,float c,enum Method m) {
			insertionPt=pt;
			cost=c;
			method=m;
		}
	
		float cost;
		float inheritedCost;
		enum Method method;
		Node* insertionPt;
	};

	// root of AABBTree
	Node* m_Root;

public:
	CAABBTree();
	~CAABBTree();

	void AddElement(const T& element); 
	bool RayIntersect(CVector3D& origin,CVector3D& dir,float& dist,T* selection);
};


template <class T>
CAABBTree<T>::CAABBTree() : m_Root(0)
{
}

template <class T>
CAABBTree<T>::~CAABBTree() 
{
	delete m_Root;
}

template <class T>
void CAABBTree<T>::AddElement(const T& element)
{
	Leaf* leaf=new Leaf;
	leaf->m_Element=element;
	leaf->m_Element->GetBounds(leaf->m_Bounds);

	if (m_Root) {
		m_Root->AddLeaf(leaf);

		// set root to topmost node
		while (m_Root->m_Parent) {
			m_Root=m_Root->m_Parent;
		}

	} else {
		m_Root=leaf;
	}
}


template <class T>
bool CAABBTree<T>::RayIntersect(CVector3D& origin,CVector3D& dir,float& dist,T* selection)
{
	if (m_Root) {
		return m_Root->RayIntersect(origin,dir,dist,selection);
	} else {
		return false;
	}
}


template <class T>
void CAABBTree<T>::Node::AddLeafAsSibling(CAABBTree<T>::Leaf* leaf)
{   
	CBound& leafBound=leaf->m_Bounds;

    if (m_Parent) {
        // recursively extend the bounding volumes of the parent nodes to allow the leaf 
        // to fit within them
		Branch* p=m_Parent;
        while (p) {
            p->m_Bounds+=leafBound;
            p=p->m_Parent;
        }
    }

    // create a new parent, whose children are the current node (this) and
    // the leaf being added
	Branch* newParent=new Branch;
    newParent->m_Parent=m_Parent;
    newParent->m_Bounds=m_Bounds;
	newParent->m_Bounds+=leafBound;

    if (m_Parent) {
        // remove the current node from it's original parent ..
        m_Parent->RemoveChild(this);
        
        // .. and add new parent as a child of the original parent
        m_Parent->AddChild(newParent);
    }

    // create new parent/child relationships
    newParent->AddChild(leaf);
	leaf->m_Parent=newParent;

    newParent->AddChild(this);
	this->m_Parent=newParent;
}

template <class T>
bool CAABBTree<T>::Branch::RayIntersect(CVector3D& origin,CVector3D& dir,float& dist,T* selection)
{
	// assume nothing hit
	bool result=false;

	// do bounds check first
	float tmin,tmax;
	if (m_Bounds.RayIntersect(origin,dir,tmin,tmax) && tmin<=dist) {
		// test all children of this branch
		for (int i=0;i<m_Children.size();i++) {
            bool hit=m_Children[i]->RayIntersect(origin,dir,dist,selection);
			if (hit) {
				// hit a triangle; can quit out ..
				result=true;
			}
        }
	}

	return result;
}

template <class T>
bool CAABBTree<T>::Leaf::RayIntersect(CVector3D& origin,CVector3D& dir,float& dist,T* selection)
{
	if (m_Element->RayIntersect(origin,dir,dist)) {
		*selection=m_Element;
		return true;
	} else {
		return false;
	}
}

//**********************************************************************************************
// FindInsertionPoint : check if the estimated cost of adding given leaf to this leaf
// is less than the current best cost; if so, update insertion data accordingly
//**********************************************************************************************
template <class T>
void CAABBTree<T>::Leaf::FindInsertionPoint(CAABBTree<T>::Leaf* leaf,CAABBTree<T>::InsertionData& inserter) 
{
	CBound thisbound=m_Bounds;
    thisbound+=leaf->m_Bounds;
	float m1=2*thisbound.GetVolume();
    if (m1<inserter.cost) 
        inserter.setInsertion(this,m1,InsertionData::ADDASSIBLING);
}

template <class T>
void CAABBTree<T>::Branch::AddLeaf(CAABBTree<T>::Leaf* leaf)
{
	// first find the node to attach the leaf to, and the method with which to attach it
    InsertionData inserter;
    FindInsertionPoint(leaf,inserter);

	// now add the leaf to the descendent found above, using the required method
    if (inserter.method==InsertionData::ADDASCHILD) {
        inserter.insertionPt->AddLeafAsChild(leaf);
	} else {
		inserter.insertionPt->AddLeafAsSibling(leaf);
	}
}

template <class T>
void CAABBTree<T>::Leaf::AddLeaf(CAABBTree<T>::Leaf* leaf)
{
    AddLeafAsSibling(leaf);
}

template <class T>
void CAABBTree<T>::Branch::AddChild(CAABBTree<T>::Node* node)
{
	m_Children.push_back(node);
}

template <class T>
void CAABBTree<T>::Branch::RemoveChild(CAABBTree<T>::Node* node)
{
	m_Children.erase(std::find(m_Children.begin(),m_Children.end(),node));
}

template <class T>
void CAABBTree<T>::Branch::FindInsertionPoint(CAABBTree<T>::Leaf* leaf,CAABBTree<T>::InsertionData& inserter) 
{
	// first count the children
	int childCt=m_Children.size();

	// get volume of union between this node and leaf
    CBound& leafBounds=leaf->m_Bounds;
	CBound uBounds=m_Bounds;
	uBounds+=leafBounds;
	float unionVol=uBounds.GetVolume();
	// get volume of this node
    float thisVol=m_Bounds.GetVolume();

	// estimate cost of adding the leaf as a sibling of this node
    float m1=2*unionVol;
    if (m1<inserter.cost) {
		// best cost so far; use this insertion point
        inserter.setInsertion(this,m1,InsertionData::ADDASSIBLING);
	}

	// estimate cost of adding the leaf as a child of this node
    float m2=(unionVol-thisVol)*childCt+thisVol;
    if (m2<inserter.cost) {
		// best cost so far; use this insertion point
        inserter.setInsertion(this,m2,InsertionData::ADDASCHILD);
	}

	// calculate cost children inherit from parent
    float inheritanceCost=inserter.inheritedCost+(unionVol-thisVol)*childCt;
    if (inheritanceCost>inserter.cost) {
		// child inheritance cost is greater than lowest cost insertion found so 
		// far, so no improvement can be found by searching children; terminate now
		return;
	}

	// now traverse through children to try and find a better insertion point 
    for (int i=0;i<m_Children.size();i++) {
        inserter.inheritedCost=inheritanceCost;
        m_Children[i]->FindInsertionPoint(leaf,inserter);
    }
}

template <class T>
void CAABBTree<T>::Branch::AddLeafAsChild(CAABBTree<T>::Leaf* leaf)
{
	Node* p=this;

	// stretch bounds of all ancestors to include the bounds of the leaf
    while (p) {
        p->m_Bounds+=leaf->m_Bounds;
        p=p->m_Parent;
    }

	// add leaf as child
    AddChild(leaf);
	leaf->m_Parent=this;
}

#endif
