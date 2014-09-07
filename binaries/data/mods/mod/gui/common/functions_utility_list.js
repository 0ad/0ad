/*
	DESCRIPTION	: Functions to manipulate objects with a 'list' property
			  (used to handle the items in list, dropdown, etc.)
	NOTES		: To ensure the selection is properly updated, it is important to
			  use these functions and not manually access the list.
*/

// ====================================================================

// Remove the item at the given index (pos) from the given list object (objectName).
function removeItem(objectName, pos)
{
	if (Engine.GetGUIObjectByName(objectName) == null)
	{
		warn("removeItem(): " + objectName + " not found.");
		return;
	}

	var list = Engine.GetGUIObjectByName(objectName).list;
	var selected = Engine.GetGUIObjectByName(objectName).selected;

	list.splice(pos, 1);

	Engine.GetGUIObjectByName(objectName).list = list;

	// It's important that we update the selection *after*
	//  we've committed the changes to the list.

	// Update the selected so the same element remains selected.
	if (selected == pos)
		Engine.GetGUIObjectByName(objectName).selected = -1;
	else if (selected > pos)
		Engine.GetGUIObjectByName(objectName).selected = selected - 1;
}

// ====================================================================

// Add the item at the given index (pos) to the given list object (objectName) with the given value (value).
function addItem(objectName, pos, value)
{
	if (Engine.GetGUIObjectByName(objectName) == null)
	{
		warn("addItem(): " + objectName + " not found.");
		return;
	}

	var list = Engine.GetGUIObjectByName(objectName).list;
	var selected = Engine.GetGUIObjectByName(objectName).selected;

	list.splice(pos, 0, value);

	Engine.GetGUIObjectByName(objectName).list = list;

	// It's important that we update the selection *after*
	//  we've committed the changes to the list.

	// Update the selected so the same element remains selected.
	if (selected >= pos)
		Engine.GetGUIObjectByName(objectName).selected = selected + 1;
}

// ====================================================================

// Adds an element to the end of the list
function pushItem(objectName, value)
{
	if (Engine.GetGUIObjectByName(objectName) == null)
	{
		warn("pushItem(): " + objectName + " not found.");
		return;
	}

	var list = Engine.GetGUIObjectByName(objectName).list;
	list.push(value);
	Engine.GetGUIObjectByName(objectName).list = list;
	// Point to the new item.
	Engine.GetGUIObjectByName(objectName).selected = getNumItems(objectName)-1;
}

// ====================================================================

// Removes the last element
function popItem(objectName)
{
	if (Engine.GetGUIObjectByName(objectName) == null)
	{
		warn("popItem(): " + objectName + " not found.");
		return;
	}

	var selected = Engine.GetGUIObjectByName(objectName).selected;
	removeItem(objectName, getNumItems(objectName)-1);

	if (selected == getNumItems(objectName)-1)
		Engine.GetGUIObjectByName(objectName).selected = -1;
}

// ====================================================================

// Retrieves the number of elements in the list
function getNumItems(objectName)
{
	if (Engine.GetGUIObjectByName(objectName) == null)
	{
		warn("getNumItems(): " + objectName + " not found.");
		return 0;
	}

	var list = Engine.GetGUIObjectByName(objectName).list;
	return list.length;
}

// ====================================================================

// Retrieves the value of the item at 'pos'
function getItemValue(objectName, pos)
{
	if (Engine.GetGUIObjectByName(objectName) == null)
	{
		warn("getItemValue(): " + objectName + " not found.");
		return "";
	}

	var list = Engine.GetGUIObjectByName(objectName).list;
	return list[pos];
}

// ====================================================================

// Retrieves the value of the currently selected item
function getCurrItemValue(objectName)
{
	if (Engine.GetGUIObjectByName(objectName) == null)
	{
		warn("getCurrItemValue(): " + objectName + " not found.");
		return "";
	}

	if (Engine.GetGUIObjectByName(objectName).selected == -1)
		return "";
	var list = Engine.GetGUIObjectByName(objectName).list;
	return list[Engine.GetGUIObjectByName(objectName).selected];
}

// ====================================================================

// Sets current item to a given string (which must be one of those
// already in the list).
function setCurrItemValue(objectName, string)
{
	if (Engine.GetGUIObjectByName(objectName) == null)
	{
		warn("setCurrItemValue(): " + objectName + " not found.");
		return -1;
	}

	if (Engine.GetGUIObjectByName(objectName).selected == -1)
		return -1;	// Return -1 if nothing selected.
	var list = Engine.GetGUIObjectByName(objectName).list;
	// Seek through list.
	for (var ctr = 0; ctr < list.length; ctr++)
	{
		// If we have found the string in the list,
		if (list[ctr] == string)
		{
			// Point selected to this item.
			Engine.GetGUIObjectByName(objectName).selected = ctr;
			return ctr;	// Return position of item.
		}
	}

	// Return -2 if failed to find value in list.
	warn("Requested string '" + string + "' not found in " + objectName + "'s list.");
	return -2;
}

// ====================================================================
