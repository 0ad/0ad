/*
 ***************************************
 functions_list.js

 Functions to manipulate objects with
 a 'list' property. It is important to
 do this and not manually to ensure that
 the selection is updated properly.
 
 ***************************************
*/

function removeItem(objectName, pos)
{
	var list = getGUIObjectByName(objectName).list;
	var selected = getGUIObjectByName(objectName).selected;
	list.splice(pos, 1);

	getGUIObjectByName(objectName).list = list;

	// It is important that we're setting the new selection *after*
	//  the list is already updated.

	// Update the selected so the same element remains selected
	if (selected == pos)
	{
		getGUIObjectByName(objectName).selected = -1;
	}
	else
	if (selected > pos)
	{
		getGUIObjectByName(objectName).selected = selected - 1;
	}
}

function addItem(objectName, pos, value)
{
	var list = getGUIObjectByName(objectName).list;
	var selected = getGUIObjectByName(objectName).selected;
	list.splice(pos, 0, value);

	getGUIObjectByName(objectName).list = list;

	// It is important that we're setting the new selection *after*
	//  the list is already updated.

	// Update the selected so the same element remains selected
	if (selected >= pos)
	{
		getGUIObjectByName(objectName).selected = selected + 1;
	}

}