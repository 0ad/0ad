/*
	DESCRIPTION	: Functions for the "online manual" part of the Session GUI.
	NOTES		: 
*/

// ====================================================================

function refreshManual()
{
	if (selection.length != 0)
	{
		// Display portrait.
		if (selection[0].traits.id.icon)
		{
			setPortrait ("mnPortrait", selection[0].traits.id.icon,
				selection[0].traits.id.civ_code, selection[0].traits.id.icon_cell);
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
		
		// Display manual text.
		manualText = getGUIObjectByName("mnText");
		manualText.caption = "[font=trebuchet14]";	

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

					manualText.caption += item + ": " + currItem + "\n";
				break;
			}
		}
		
		if (selection[0].actions)
		{
			manualText.caption += "[font=trebuchet16][[Actions]]\n";

			subContainer = selection[0].actions;
			for (sub in subContainer)
			{
				switch (sub)
				{
					default:
						// List attribute.
						manualText.caption += sub;
						// List value if there is one.
						if (subContainer[sub] && subContainer[sub] != "true" && subContainer[sub] != true)
							manualText.caption += ": " + subContainer[sub].toString() + "\n";
						else
							manualText.caption += "\n"
					break;
				}
				
				
			}
		}
		
		manualText.caption += "[/font]";
	}
	else
	{
		// If the player has removed the selection while the manual is open, close the manual.
		guiHide("mn");
		guiUnHide("sn");
	}
}

// ====================================================================