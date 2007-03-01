/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/**
	@file FCDSceneNodeIterator.h
	This file contains the FCDSceneNodeIterator class.
*/

#ifndef _FCD_SCENE_NODE_ITERATOR_H_
#define _FCD_SCENE_NODE_ITERATOR_H_

class FCDSceneNode;

typedef fm::pvector<FCDSceneNode> FCDSceneNodeList;

/** This class is used to process a given scene node and its full sub-tree.
	Currently, this class does not care whether multiple instances of the same node is processed. */
class FCDSceneNodeIterator
{
private:
	FCDSceneNodeList queue;
	size_t iterator;

public:
	/** Constructor.
		@param root The scene root of the sub-tree to iterate over. */
	FCDSceneNodeIterator(FCDSceneNode* root);

	/** Destructor. */
	~FCDSceneNodeIterator();

	/** Retrieves the current node to process.
		@return The current node. */
	FCDSceneNode* GetNode();

	/** Advances the iteration pointer and retrieves the next node to process.
		@return The node to process. */
	FCDSceneNode* Next();

	/** Retrieves whether the full sub-tree has been processed. */
	inline bool IsDone() { return iterator >= queue.size(); }

	/** Advances the iteration pointer.
		@return The iterator. */
	inline FCDSceneNodeIterator& operator++() { Next(); return (*this); }

	/** Retrieves the current node to process.
		@return The current node. */
	inline FCDSceneNode* operator*() { return GetNode(); }
};

#endif // _FCD_SCENE_NODE_ITERATOR_H_
