/*
	DESCRIPTION	: Functions to manipulate objects with a 'list' property
			  (used to handle the items in list, dropdown, etc.)
	NOTES		: To ensure the selection is properly updated, it is important to
			  use these functions and not manually access the list.
*/

// ====================================================================

// Remove the item at the given index (pos) from the given list object (objectName).
function removeItem (objectName, pos)
{
	if (getGUIObjectByName (objectName) == null)
		console.write ("removeItem(): " + objectName + " not found.");

	var list = getGUIObjectByName (objectName).list;
	var selected = getGUIObjectByName (objectName).selected;

	list.splice(pos, 1);

	getGUIObjectByName (objectName).list = list;

	// It's important that we update the selection *after*
	//  we've committed the changes to the list.

	// Update the selected so the same element remains selected.
	if (selected == pos)
	{
		getGUIObjectByName (objectName).selected = -1;
	}
	else
	if (selected > pos)
	{
		getGUIObjectByName (objectName).selected = selected - 1;
	}
}

// ====================================================================

// Add the item at the given index (pos) to the given list object (objectName) with the given value (value).
function addItem (objectName, pos, value)
{
	if (getGUIObjectByName (objectName) == null)
		console.write ("addItem(): " + objectName + " not found.");

	var list = getGUIObjectByName (objectName).list;
	var selected = getGUIObjectByName (objectName).selected;

	list.splice (pos, 0, value);

	getGUIObjectByName (objectName).list = list;

	// It's important that we update the selection *after*
	//  we've committed the changes to the list.

	// Update the selected so the same element remains selected.
	if (selected >= pos)
	{
		getGUIObjectByName (objectName).selected = selected + 1;
	}
}

// ====================================================================

// Adds an element to the end of the list
function pushItem (objectName, value)
{
	if (getGUIObjectByName (objectName) == null)
		console.write ("pushItem(): " + objectName + " not found.");

	var list = getGUIObjectByName (objectName).list;
	list.push (value);
	getGUIObjectByName (objectName).list = list;
	// Point to the new item.
	getGUIObjectByName(objectName).selected = getNumItems(objectName)-1;
}

// ====================================================================

// Removes the last element
function popItem (objectName)
{
	if (getGUIObjectByName (objectName) == null)
		console.write ("popItem(): " + objectName + " not found.");

	var selected = getGUIObjectByName (objectName).selected;
	removeItem(objectName, getNumItems(objectName)-1);

	if (selected == getNumItems(objectName)-1)
	{
		getGUIObjectByName(objectName).selected = -1;
	}
}

// ====================================================================

// Retrieves the number of elements in the list
function getNumItems (objectName)
{
	if (getGUIObjectByName (objectName) == null)
		console.write ("getNumItems(): " + objectName + " not found.");

	var list = getGUIObjectByName(objectName).list;
	return list.length;
}

// ====================================================================

// Retrieves the value of the item at 'pos'
function getItemValue (objectName, pos)
{
	if (getGUIObjectByName (objectName) == null)
		console.write ("getItemValue(): " + objectName + " not found.");

	var list = getGUIObjectByName(objectName).list;
	return list[pos];
}

// ====================================================================

// Retrieves the value of the currently selected item
function getCurrItemValue (objectName)
{
	if (getGUIObjectByName (objectName) == null)
		console.write ("getCurrItemValue(): " + objectName + " not found.");

	if (getGUIObjectByName(objectName).selected == -1)
		return "";
	var list = getGUIObjectByName(objectName).list;
	return list[getGUIObjectByName(objectName).selected];
}

// ====================================================================

// Sets current item to a given string (which must be one of those
// already in the list).
function setCurrItemValue (objectName, string)
{
	if (getGUIObjectByName(objectName) == null) {
		console.write ("setCurrItemValue(): " + objectName + " not found.");
		return -1;
	}

	if (getGUIObjectByName(objectName).selected == -1)
		return -1;	// Return -1 if nothing selected.
	var list = getGUIObjectByName(objectName).list;
	// Seek through list.
	for (ctr = 0; ctr < list.length; ctr++)
	{
		// If we have found the string in the list,
		if (list[ctr] == string)
		{
			// Point selected to this item.
			getGUIObjectByName(objectName).selected = ctr;
			return ctr;	// Return position of item.
		}
	}

	// Return -2 if failed to find value in list.
	console.write ("Requested string '" + string + "' not found in " + objectName + "'s list.");
	return -2;
}

// ====================================================================