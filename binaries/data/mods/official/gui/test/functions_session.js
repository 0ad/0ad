function initSession()
{
	initCoord();
	initGroupPane();
	initResourcePool();
}

// ====================================================================

function setPortrait(objectName, portraitString) 
{
	// Use this function as a shortcut to change a portrait object to a different portrait image. 

	// Accepts an object and specifies its default, rollover (lit) and disabled (grey) sprites.
	// Sprite Format: "ui_portrait_"portraitString"_64"
	// Sprite Format: "ui_portrait_"portraitString"_64""-lit"
	// Sprite Format: "ui_portrait_"portraitString"_64""-grey"
	// Note: Make sure the file follows this naming convention or bad things could happen.

        // Get GUI object
        GUIObject = getGUIObjectByName(objectName);

	// Set the three portraits.
	GUIObject.sprite = "ui_portrait_" + portraitString + "_64";
	// Note we need to use a special syntax here (object["param"] instead of object.param because dashes aren't actually in JS's variable-naming conventions.
	GUIObject["sprite-over"] = GUIObject.sprite + "-lit";
	GUIObject["sprite-disabled"] = GUIObject.sprite + "-grey";
}

// ====================================================================

function getObjectInfo() 
{
	// Updated each tick to extract entity information from selected unit(s).

	if (!selection.length) 	// If no entity selected,
	{
		// Hide Status Orb
		getGUIObjectByName("session_status_orb").hidden = true;

		// Hide Group Pane.
		getGUIObjectByName("session_group_pane").hidden = true;

		getGlobal().MultipleEntitiesSelected = 0;
	}
	else			// If at least one entity selected,
	{
		// Store globals for entity information.
//		strString = "" + selection[0].position;
//		EntityPos = strString.substring(20,strString.length-3);

		UpdateStatusOrb();

	        // Check if a group of entities selected
	        if (selection.length > 1) 
		{
			// If a group pane isn't already open, and we don't have the same set as last time,
			// NOTE: This "if" is an optimisation because the game crawls if this set of processing occurs every frame.
			// It's quite possible for the player to select another group of the same size and for it to not be recognised.
			// Best solution would be to base this off a "new entities selected" instead of an on-tick.
			if (
				// getGUIObjectByName("session_group_pane").hidden == true || 
				selection.length != getGlobal().MultipleEntitiesSelected)
			{
				UpdateGroupPane();
		                getGlobal().MultipleEntitiesSelected = selection.length;
			}
	        } 
		else
		{
	                getGlobal().MultipleEntitiesSelected = 0;

			// Hide Group Pane.
			getGUIObjectByName("session_group_pane").hidden = true;
		}
        }

}

// ====================================================================

function MakeUnit(x, y, z, MakeUnitName)
{
	// Spawn an entity at the given coordinates.

	DudeSpawnPoint = new Vector3D(x, y, z);
	new Entity(getEntityTemplate(MakeUnitName), DudeSpawnPoint, 1.0);
	// writeConsole(MakeUnitName + " created at " + DudeSpawnPoint);
}

// ====================================================================

function selected()
{
	// Returns how many units selected.

	if( selection.length > 0 )
		return( selection[0] );
	return( null );
}
