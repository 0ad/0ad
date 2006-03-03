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
			manualRollover = getGUIObjectByName("mnRollover");
			manualRollover.caption = selection[0].traits.id.rollover;
		}	
		
		// Display history text.
		if (selection[0].traits.id.rollover)
		{
			manualHistory = getGUIObjectByName("mnHistory");
			manualHistory.caption = selection[0].traits.id.history;
		}	
		
		// Build manual text.
		manualText = "";	
		addItemsRecursively ( selection[0], "", "trebuchet14" );
		manualText += recursiveString;
		
/*		
		container = selection[0];
		for( item in container )
		{
			switch (item)
			{
				case "actions":
				case "traits":
				break;
				default:
					// If it's a null value, or function reference, don't even bother.
					if (item == null || container[item] == null) break;
					
					// Store current property value.
					currItem = container[item].toString();
					
					// If it's a function reference, also don't bother.
					if (currItem[0] == "\n") break;

					manualText += item + ": " + currItem + "\n";
				break;
			}
		}
		
		if (selection[0].actions)
		{
			manualText += "[font=trebuchet14b][[Actions]]\n";

			subContainer = selection[0].actions;
			for (sub in subContainer)
			{
				switch (sub)
				{
					default:
						// List attribute.
						manualText += sub;
						// List value if there is one.
						if (subContainer[sub] && subContainer[sub] != "true" && subContainer[sub] != true)
							manualText += ": " + subContainer[sub].toString() + "\n";
						else
							manualText += "\n"
					break;
				}
				
				
			}
		}
*/		
		
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