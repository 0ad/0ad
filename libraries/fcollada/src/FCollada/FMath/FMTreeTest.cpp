/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMTree.h"
#include "FUtils/FUTestBed.h"

////////////////////////////////////////////////////////////////////////
static bool IsEquivalent(const fm::tree<uint32, bool>& tree, uint32* keys, size_t keyCount)
{
	fm::tree<uint32, bool>::const_iterator it;
	size_t index = 0;
	for (it = tree.begin(); it != tree.end() && index < keyCount; ++it, ++index)
	{
		if ((*it).first != keys[index]) return false;
	}
	return index == keyCount && it == tree.end();
}

////////////////////////////////////////////////////////////////////////
TESTSUITE_START(FMTree)

TESTSUITE_TEST(0, Iterator)
	// Create an empty tree.
	fm::tree<uint32, bool> tree;
	PassIf(tree.begin() == tree.end());
	PassIf(tree.empty());
	PassIf(tree.size() == 0);

	// Insert a first element.
	fm::tree<uint32, bool>::iterator it = tree.insert(35, false);
	PassIf((*it).first == 35);
	PassIf((*it).second == false);
	it = tree.begin();
	PassIf((*it).first == 35);
	PassIf((*it).second == false);
	++it;
	PassIf(it == tree.end());

	// Insert a second element, verify the order.
	it = tree.insert(15, true);
	PassIf((*it).first == 15);
	PassIf((*it).second == true);
	it = tree.begin();
	PassIf((*it).first == 15);
	PassIf((*it).second == true);
	++it;
	PassIf((*it).first == 35);
	PassIf((*it).second == false);
	++it;
	PassIf(it == tree.end());

	// Insert a third element and verify that the tree is still ordered.
	it = tree.insert(25, false);
	PassIf((*it).first == 25);
	it = tree.begin();
	PassIf((*it).first == 15);
	++it;
	PassIf((*it).first == 25);
	++it;
	PassIf((*it).first == 35);
	++it;
	PassIf(it == tree.end());

	// Verify that look-ups work.
	it = tree.find(15);
	PassIf((*it).first == 15);
	PassIf((*it).second == true);
	it = tree.find(35);
	PassIf((*it).first == 35);
	PassIf((*it).second == false);
	it = tree.find(20);
	PassIf(it == tree.end());
	it = tree.find(25);
	PassIf((*it).first == 25);
	PassIf((*it).second == false);

	// Check out the const_iterator.
	const fm::tree<uint32, bool>& ctree = tree;
	fm::tree<uint32, bool>::const_iterator cit = ctree.find(15);
	PassIf((*cit).first == 15);
	PassIf((*cit).second == true);
	++cit;
	PassIf((*cit).first == 25);
	PassIf((*cit).second == false);
	++cit;
	PassIf((*cit).first == 35);
	PassIf((*cit).second == false);
	++cit;
	PassIf(cit == ctree.end());
	PassIf(cit == tree.end());

	// Check the backward operators.
	cit = tree.last();
	PassIf((*cit).first == 35);
	--cit;
	PassIf((*cit).first == 25);
	--cit;
	PassIf((*cit).first == 15);
	--cit;
	PassIf(cit == tree.end());

	// Clear this tree.
	tree.clear();
	PassIf(tree.begin() == tree.end());
	PassIf(tree.last() == tree.end());
	PassIf(tree.empty());
	PassIf(tree.size() == 0);

	// Verify that we can still use a cleared tree.
	it = tree.insert(14, false);
	PassIf(it != tree.end());
	it = tree.find(14);
	PassIf(it != tree.end());
	PassIf(it->first == 14);
	PassIf(it->second == false);
	PassIf(tree.size() == 1);

TESTSUITE_TEST(1, Containment)
	// Create a tree with many elements.
	fm::tree<uint32, bool> tree;
	fm::tree<uint32, bool>::iterator it;
	static uint32 unorderedKeys[15] = { 17, 3, 99, 42, 16, 22, 75, 59, 0, 4, 25, 77, 7, 14, 62 };
	for (size_t i = 0; i < 15; ++i)
	{
		tree.insert(unorderedKeys[i], false);
	}

	// Verify that the tree elements are ordered.
	static uint32 orderedKeys[15] = { 0, 3, 4, 7, 14, 16, 17, 22, 25, 42, 59, 62, 75, 77, 99 };
	PassIf(IsEquivalent(tree, orderedKeys, 15));

	// Verify that inserting existing elements doesn't change the order, but changes the values.
	tree.insert(16, true);
	PassIf(IsEquivalent(tree, orderedKeys, 15));
	tree.insert(22, true);
	PassIf(IsEquivalent(tree, orderedKeys, 15));
	it = tree.find(22);
	PassIf((*it).first == 22);
	PassIf((*it).second == true);
	it = tree.find(16);
	PassIf((*it).first == 16);
	PassIf((*it).second == true);
	
	// Remove one element and verify that the ordering and the sizing are good.
	static uint32 orderedKeys2[14] = { 0, 3, 4, 7, 14, 16, 17, 25, 42, 59, 62, 75, 77, 99 };
	PassIf(tree.size() == 15);
	tree.erase(22);
	PassIf(IsEquivalent(tree, orderedKeys2, 14));
	PassIf(tree.size() == 14);
	tree.insert(22, true);
	PassIf(IsEquivalent(tree, orderedKeys, 15));
	PassIf(tree.size() == 15);

TESTSUITE_TEST(2, Copy)
	// Create a tree with many elements.
	fm::tree<size_t, fm::tree<size_t, bool> > tree;
	static size_t unorderedKeys[15] = { 17, 3, 99, 42, 16, 22, 75, 59, 0, 4, 25, 77, 7, 14, 62 };
	for (size_t i = 0; i < 15; ++i)
	{
		fm::tree<size_t, bool> innerTree;
		for (size_t j = 0; j < unorderedKeys[i] / 10; j++)
		{
			innerTree.insert(j, false);
		}
		tree.insert(unorderedKeys[i], innerTree);
	}

	fm::tree<size_t, fm::tree<size_t, bool> > copyTree;
	copyTree = tree;
	FailIf(&tree == &copyTree);

	FailIf(tree.size() != copyTree.size());
	for (size_t i = 0; i < 15; i++)
	{
		fm::tree<size_t, bool>& copyList = copyTree[unorderedKeys[i]];
		fm::tree<size_t, bool>& list = tree[unorderedKeys[i]];
		FailIf(&copyList == &list);
		FailIf(copyList.size() != list.size());

		for (size_t j = 0; j < copyList.size(); j++)
		{
			FailIf(&copyList[j] == &list[j]);
			FailIf(copyList[j] != list[j]);
		}
	}


TESTSUITE_END

