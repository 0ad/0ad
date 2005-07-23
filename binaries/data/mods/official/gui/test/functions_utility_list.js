/*
 ***************************************
 functions_global.js

 Functions to manipulate objects with
 a 'list' property. It is important to
 do this and not manually to ensure that
 the selection is updated properly.

 ***************************************
*/

function removeItem (objectName, pos)
{
	// Remove the item at the given index (pos) from the given list object (objectName).

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

function addItem (objectName, pos, value)
{
	// Add the item at the given index (pos) to the given list object (objectName) with the given value (value).

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