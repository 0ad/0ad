
#ifndef CGUIList_H
#define CGUIList_H

class CGUIList
{
public: // struct:ish (but for consistency I call it _C_GUIList, and
		//  for the same reason it is a class, so that confusion doesn't
		//  appear when forward declaring.
	/**
	 * List of items (as text), the post-processed result is stored in
	 *  the IGUITextOwner structure of this class.
	 */
	std::vector<CGUIString> m_Items;
};

#endif