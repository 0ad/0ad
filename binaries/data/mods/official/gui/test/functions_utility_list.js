/*
 ***************************************
 functions_global.js

 Functions for the objects having the
 'list' property. These functions also
 updates the selection correctly.

 ***************************************
*/

function removeItem(objectName, pos)
{
	var list = getGUIObjectByName(objectName).list;
	var selected = getGUIObjectByName(objectName).selected;

	list.splice(pos, 1);

	getGUIObjectByName(objectName).list = list;

	// It's important that we update the selection *after*
	//  we've committed the changes to the list.

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

	// It's important that we update the selection *after*
	//  we've committed the changes to the list.

	if (selected >= pos)
	{
		getGUIObjectByName(objectName).selected = selected + 1;
	}
}