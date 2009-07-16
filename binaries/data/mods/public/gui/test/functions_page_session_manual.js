/*
	DESCRIPTION	: Functions for the "online manual" part of the Session GUI.
	NOTES		: 
*/

// ====================================================================

function addItemsRecursively (container, indent, fontName)
{
	// List the tree of items in a container and store it in the string *recursiveString* by recursively calling the function. (Thanks, Philip.)

	// Reset recursive string if this is the first call.
	if (indent == "")
		recursiveString = "";

	for (var item in container)
	{
		// Skip null strings and function references.
		if (item != null && container[item] != null
			&& container[item].toString().substring (0, 1) != "\n" && item != "template")
		{
			// Root-level items are in bold.
			recursiveString += "[font=" + fontName;
			if (indent == "")
				recursiveString += "b]";
			else
				recursiveString += "]";
		
			// Display the property name, and the value, if any.
			if (container[item] == true && container[item].toString() != true)
				recursiveString += indent + " " + item + "\n";
			else
				recursiveString += indent + " " + item + ": " + container[item] + "\n";

			// Retrieve sub-items.
			if (typeof container[item] == 'object')
				addItemsRecursively (container[item], indent + "--", fontName);			
		}
	}
}

// ====================================================================

function refreshManual()
{
	if (selection.length != 0)
	{
		// Display portrait.
		if (selection[0].traits.id.icon)
		{
			setPortrait ("mnPortrait", selection[0].traits.id.icon,
				"Button" + toTitleCase(selection[0].traits.id.civ_code), selection[0].traits.id.icon_cell);
		}
		
		// Display rollover text.
		if (selection[0].traits.id.rollover)
		{
			var manualRollover = getGUIObjectByName("mnRollover");
			manualRollover.caption = selection[0].traits.id.rollover;
		}	
		
		// Display history text.
		if (selection[0].traits.id.rollover)
		{
			var manualHistory = getGUIObjectByName("mnHistory");
			manualHistory.caption = selection[0].traits.id.history;
		}	
		
		// Build manual text.
		var manualText = "";	
		addItemsRecursively ( selection[0], "", "trebuchet14" );
		manualText += recursiveString;
		
		// Close the manual string.
		manualText += "[/font]";
		
		// Assign manual text to control.
		getGUIObjectByName("mnText").caption = manualText;
	}
	else
	{
		// If the player has removed the selection while the manual is open, close the manual.
		guiHide("mn");
		guiUnHide("sn");
	}
}

// ====================================================================