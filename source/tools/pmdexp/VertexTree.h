#ifndef __VERTEXTREE_H
#define __VERTEXTREE_H

// necessary includes
#include "ExpVertex.h"

////////////////////////////////////////////////////////////////////////////////////////////
// VertexTree: template tree class for building unique vertices in varying fashions; the
// template parameter Cmp specifies a function which compares (and possibly modifies) 
// two vertices 
// FIXME:  ugh .. modifying a vertex already in the tree may cause it to be in the 
// wrong position in the tree 
template <class Cmp>
class VertexTree
{
private:
	struct Node {
		Node(int index,ExpVertex& vtx) : _index(index), _vertex(vtx), _left(0), _right(0) {}

		// index into the output _vertices array
		int _index;
		// reference to actual vertex on the node (vertex itself in _vertices array)
		ExpVertex& _vertex;
		// children
		Node* _left;
		Node* _right;
	};

public:
	VertexTree(VertexList& vertices) : _vertices(vertices), _treeroot(0) {}

	int insert(const ExpVertex& vtx) {
		// copy incoming vertex in case the comparison function wants to modify
		// it before storing it ..
		ExpVertex copy=vtx;
		if (_treeroot) {
			return insert(_treeroot,copy);
		} else {
			_vertices.push_back(copy);
			_treeroot=new Node(0,_vertices.back());
			return 0;
		}
	}

	int insert(Node* node,ExpVertex& vtx) {
		// compare given element with element at given node
		Cmp compareFn;
		int cmp=compareFn(node->_vertex,vtx);
		if (cmp==0) {
			// matching vertex found
			return node->_index;
		} else {
			if (cmp==-1) {
				// this node less than new node
				if (node->_left) {
					// send down left tree
					return insert(node->_left,vtx);
				} else {
					// no left node - create one
					_vertices.push_back(vtx);
					node->_left=new Node(_vertices.size()-1,_vertices.back());
					return _vertices.size()-1;
				}
			} else {
				// this node greater than new node
				if (node->_right) {
					// send down right tree
					return insert(node->_right,vtx);
				} else {
					// no right node - create one
					_vertices.push_back(vtx);
					node->_right=new Node(_vertices.size()-1,_vertices.back());
					return _vertices.size()-1;
				}
			}
		}
	}

private:
	Node* _treeroot;
	VertexList& _vertices;
};


class UniqueVertexCmp {
public:
	int operator()(ExpVertex& left,ExpVertex& right) {
		// check distance between two vertices ..
		CVector3D vec3=left.m_Pos-right.m_Pos;
		if (vec3.GetLength()>0.0001f) {
			// vertices too far apart to weld .. sort on x,y,z
			if (left.m_Pos[0]<right.m_Pos[0]) {
				return -1;
			} else if (left.m_Pos[0]>right.m_Pos[0]) {
				return 1;
			} else {
				if (left.m_Pos[1]<right.m_Pos[1]) {
					return -1;
				} else if (left.m_Pos[1]>right.m_Pos[1]) {
					return 1;
				} else {
					if (left.m_Pos[2]<right.m_Pos[2]) {
						return -1;
					} else {
						return 1;
					}		 
				}
			}
		} else {
			// weld two points together ..
			right.m_Pos=left.m_Pos;

			// .. and now compare by texcoords
			CVector3D vec2(left.m_UVs[0]-right.m_UVs[0],left.m_UVs[1]-right.m_UVs[1],0);
			if (vec2.GetLength()>0.0001f) {
				// uvs too far apart to weld .. sort on u,v
				if (left.m_UVs[0]<right.m_UVs[0]) {
					return -1;
				} else if (left.m_UVs[0]>right.m_UVs[0]) {
					return 1;
				} else {
					if (left.m_UVs[1]<right.m_UVs[1]) {
						return -1;
					} else {
						return 1;
					}	  
				}
			} else {
				// weld uvs
				right.m_UVs[0]=left.m_UVs[0];
				right.m_UVs[1]=left.m_UVs[1];

				// compare normals
				if (left.m_Normal[0]<right.m_Normal[0]) {
					return -1;
				} else if (left.m_Normal[0]>right.m_Normal[0]) {
					return 1;
				} else {
					if (left.m_Normal[1]<right.m_Normal[1]) {
						return -1;
					} else if (left.m_Normal[1]>right.m_Normal[1]) {
						return 1;
					} else {
						if (left.m_Normal[2]<right.m_Normal[2]) {
							return -1;
						} else if (left.m_Normal[2]>right.m_Normal[2]) {
							return 1;
						} else {
							return 0;
						}
					}
				}
			}
		}
	}
};

#endif