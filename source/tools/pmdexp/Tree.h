#ifndef __TREE_H
#define __TREE_H

//////////////////////////////////////////////////////////////////////////////
// Tree: template class to build a binary tree of elements of class T
template <class T,class Cmp>
class Tree 
{
public:
	template <class T>
	class Node {
	public:
		Node<T>* _Next;
	
	public:
		Node() : _Left(0), _Right(0) {}
		
		int _Index;
		T _Element;
		Node<T>* _Left;
		Node<T>* _Right;
	};


public:
	Tree();
	~Tree();

	void Clear();

	int Find(T& element);
	int Find(Node<T>* _node,T& _vtx);

	int Add(T& element);
	int Add(Node<T>* _node,T& _vtx,int _index);

	// total nodes currently in tree
	int Entries() { return _Size; }

	void Reserve(int count) { _AllocatedNodes.reserve(count); }

	T& operator[](int index) { return _AllocatedNodes[index]->_Element; }

private:
	// the root node of the tree
	Node<T>* _Root;
	// all allocated nodes in tree
	std::vector<Node<T>*> _AllocatedNodes;
	// size of tree
	int _Size;
	// the comparison function used to compare two elements in the tree
	Cmp _Cmp;
};

//***************************************************************************
// Default Tree constructor - create tree with no allocated nodes
//***************************************************************************
template <class T,class Cmp>
Tree<T,Cmp>::Tree()
{
	_Root=0;
	_Size=0;
}


//***************************************************************************
// Tree destructor
//***************************************************************************
template <class T,class Cmp>
Tree<T,Cmp>::~Tree()
{
	Clear();
}


template <class T,class Cmp>
void Tree<T,Cmp>::Clear()
{
	for (int i=0;i<_AllocatedNodes.Entries();i++) {
		_NodePile.Release(_AllocatedNodes[i]);
	}
	_AllocatedNodes.SetSize(0);
	_Root=0;
	_Size=0;
}

//***************************************************************************
// Add : insert an element into the tree; return the index of the treenode
// at which the element was added, or, if an identical element was already
// in the tree, return it's index
//***************************************************************************
template <class T,class Cmp>
int Tree<T,Cmp>::Add(T& element) 
{
	if (_Root) {
		int index=Add(_Root,element,_Size);
		if (index==_Size) {
			// element added to tree
			_Size++;
		} else {
			// element not added
		}
		return index;
	} else {
		_Root=_NodePile.Allocate();
		_AllocatedNodes.Add(_Root);
		_Root->_Element=element;
		_Root->_Index=0;
		_Root->_Left=0;
		_Root->_Right=0;
		_Size++;
		return 0;
	}
}


//***************************************************************************
// Add : insert an element into the given node; return the index of the 
// treenode at which the element was added, or, if an identical element was 
// already in the tree, return it's index
//***************************************************************************
template <class T,class Cmp>
int Tree<T,Cmp>::Add(Node<T>* _node,T& element,int _index)
{
	// compare given element with element at given node
	int cmp=_Cmp.compare(_node->_Element,element);
	if (cmp==0) {
		// identical - return index of this node
		return _node->_Index;
	} else {
		if (cmp==-1) {
			// this node less than new node
			if (_node->_Left) {
				// send down left tree
				return Add(_node->_Left,element,_index);
			} else {
				// no left node - create one
				_node->_Left=_NodePile.Allocate();
				_AllocatedNodes.Add(_node->_Left);
				_node->_Left->_Element=element;
				_node->_Left->_Index=_index;
				_node->_Left->_Left=0;
				_node->_Left->_Right=0;
				return _index;
			}
		} else {
			// this node greater than new node
			if (_node->_Right) {
				// send down right tree
				return Add(_node->_Right,element,_index);
			} else {
				// no right node - create one
				_node->_Right=_NodePile.Allocate();
				_AllocatedNodes.Add(_node->_Right);
				_node->_Right->_Element=element;
				_node->_Right->_Index=_index;
				_node->_Right->_Left=0;
				_node->_Right->_Right=0;
				return _index;
			}
		}
	}
}

//***************************************************************************
// Find: try and find a matching element in the tree; return the index of the 
// treenode at which a match was found, or -1 if no match found
//***************************************************************************
template <class T,class Cmp>
int Tree<T,Cmp>::Find(T& element) 
{
	return _Root ? Find(_Root,element) : -1;
}


//***************************************************************************
// Find: try and find a matching element in the given node; return the index 
// of the treenode at which a match was found, or -1 if no match found
//***************************************************************************
template <class T,class Cmp>
int Tree<T,Cmp>::Find(Node<T>* _node,T& element)
{
	// compare given element with element at given node
	int cmp=_Cmp.compare(_node->_Element,element);
	if (cmp==0) {
		// identical - return index of this node
		return _node->_Index;
	} else {
		if (cmp==-1) {
			// this node less than new node
			if (_node->_Left) {
				// send down left tree
				return Find(_node->_Left,element);
			} else {
				// no left node - no match on this subtree
				return -1;
			}
		} else {
			// this node greater than new node
			if (_node->_Right) {
				// send down right tree
				return Find(_node->_Right,element);
			} else {
				// no left node - no match on this subtree
				return -1;
			}
		}
	}
}


#endif