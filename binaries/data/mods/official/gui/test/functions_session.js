function initSession()
{
	GUIType="top";
	GUIStyleName = new Array();
	GUIStyleSize1 = new Array();
	GUIStyleSize2 = new Array();
	GUIStyle_Last = 0;
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
	// Note we need to use a special syntax here (object["param"] instead of object.param) because dashes aren't actually in JS's variable-naming conventions.
	GUIObject["sprite-over"] = GUIObject.sprite + "-lit";
	GUIObject["sprite-disabled"] = GUIObject.sprite + "-grey";
}

// ====================================================================

function setSize(objectName, sizeString)
{
	// Use this function as a shortcut to change the size of a GUI control. 

        // Get GUI object
        GUIObject = getGUIObjectByName(objectName);

	// Set size
	GUIObject.size = sizeString;
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
	}
	else			// If at least one entity selected,
	{
		// Store globals for entity information.
//		EntityName = selection[0].traits.id.generic;
//		strString = "" + selection[0].position;
//		EntityPos = strString.substring(20,strString.length-3);

		// Update portrait
		setPortrait("session_panel_status_portrait", selection[0].traits.id.icon);

		// Reveal Status Orb
		getGUIObjectByName("session_status_orb").hidden = false;

	        // Check if a group of entities selected
	        if (selection.length > 1) 
		{
			// If a group pane isn't already open, and we don't have the same set as last time,
			// NOTE: This "if" is an optimisation because the game crawls if this set of processing occurs every frame.
			// It's quite possible for the player to select another group of the same size and for it to not be recognised.
			// Best solution would be to base this off a "new entities selected" instead of an on-tick.
			if (getGUIObjectByName("session_group_pane").hidden == true || selection.length != MultipleEntitiesSelected)
			{

				// Reveal Group Pane.
				getGUIObjectByName("session_group_pane").hidden = false;

				// Set size of Group Pane background.
				if (selection.length <= 13)
				{
					switch (GUIType)
					{
						case "top":
							setSize("session_group_pane_bg", "0%+20 0% 100%-20 0%+86");
						break;
						case "bottom":
							setSize("session_group_pane_bg", "0%+20 100%-86 100%-20 100%");
						break;
					}
				}
				else
				if (selection.length > 13 && selection.length < 26)
				{
					switch (GUIType)
					{
						case "top":
							setSize("session_group_pane_bg", GUIStyleSize1[FlipGUILoop]);
						break;
						case "bottom":
							setSize("session_group_pane_bg", GUIStyleSize2[FlipGUILoop]);
						break;
					}
				}
				else
				{
					switch (GUIType)
					{
						case "top":
							setSize("session_group_pane_bg", "0%+20 0% 100%-20 0%+168");
						break;
						case "bottom":
							setSize("session_group_pane_bg", "0%+20 100%-168 100%-20 100%");
						break;
					}
				}

				// Display appropriate portraits.						
				for (groupPaneLoop = 1; groupPaneLoop <= 39; groupPaneLoop++)
				{
					groupPanePortrait = getGUIObjectByName("session_group_pane_portrait_" + groupPaneLoop);
					groupPaneBar = getGUIObjectByName("session_group_pane_portrait_" + groupPaneLoop + "_bar");

					// If it's a valid entity,
					if (groupPaneLoop <= selection.length){
						// Reveal and set to display this entity's portrait in the group pane.
						groupPanePortrait.hidden = false;
						groupPaneBar.hidden = false;
						setPortrait("session_group_pane_portrait_" + groupPaneLoop, selection[groupPaneLoop-1].traits.id.icon);
					}
					// If it's empty, hide its group portrait.
					else
					{
						groupPanePortrait.hidden = true;
						groupPaneBar.hidden = true;
					}

				}

		                MultipleEntitiesSelected = selection.length;
			}
	        } 
		else
		{
	                MultipleEntitiesSelected = 0;

			// Hide Group Pane.
			getGUIObjectByName("session_group_pane").hidden = true;
		}
        }
}

// ====================================================================

function AddSize(objectName, objectSize1, objectSize2)
{
	// Used to store the two GUI style sizes for an object on creation.
	// Used later by FlipGUI() to switch the objects to a new set of positions.

	GUIStyleName[GUIStyle_Last] = objectName;
	GUIStyleSize1[GUIStyle_Last] = objectSize1;
	GUIStyleSize2[GUIStyle_Last] = objectSize2;

//	writeConsole("Added " + GUIStyle_Last + ": " + GUIStyleName[GUIStyle_Last] + " (" + GUIStyleSize1[GUIStyle_Last] + ", " + GUIStyleSize2[GUIStyle_Last] + ").");

	GUIStyle_Last++; // Increment counter for next entry.
}

// ====================================================================

function FlipGUI(NewGUIType)
{
	// Sets the GUI coordinates and graphics so that the panel is either at the top or bottom of the screen.

	switch (NewGUIType)
	{
		// Set which GUI to use.
		case "top":
		case "bottom":
		case "none":
			GUIType=NewGUIType;
		break;
		default:
			// If no type specified, toggle.
			if (GUIType == "top")
				GUIType = "bottom";
			else
			if (GUIType == "bottom")
				GUIType = "none";
			else
				GUIType = "top";
		break;
	}

	if (GUIType != "none")
	{
		GUIObjectUnhide("session_gui");
		GUIObjectUnhide("always_on");

		// Seek through all sizes created.
		for (FlipGUILoop = 0; FlipGUILoop < GUIStyle_Last; FlipGUILoop++)
		{
			// Set each object to the other size.
			switch (GUIType)
			{
				case "top":
					setSize(GUIStyleName[FlipGUILoop], GUIStyleSize1[FlipGUILoop]);
					switch (GUIStyleName[FlipGUILoop]){
						case "session_panel_minimap_segbottom1":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = GUIStyleName[FlipGUILoop];
						break;
						case "session_panel_minimap_segbottom2":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = GUIStyleName[FlipGUILoop];
						break;
						case "session_panel_minimap_segbottom3":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = GUIStyleName[FlipGUILoop];
						break;
						case "session_panel_minimap_segbottom4":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = GUIStyleName[FlipGUILoop];
						break;
						case "session_panel_status_bg":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = "session_panel_status_bg-top";
						break;
						default:
						break;
					}
				break;
				case "bottom":
					setSize(GUIStyleName[FlipGUILoop], GUIStyleSize2[FlipGUILoop]);
					switch (GUIStyleName[FlipGUILoop]){
						case "session_panel_minimap_segbottom1":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = "session_panel_minimap_segtop1";
						break;
						case "session_panel_minimap_segbottom2":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = "session_panel_minimap_segtop2";
						break;
						case "session_panel_minimap_segbottom3":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = "session_panel_minimap_segtop3";
						break;
						case "session_panel_minimap_segbottom4":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = "session_panel_minimap_segtop4";
						break;
						case "session_panel_status_bg":
							getGUIObjectByName(GUIStyleName[FlipGUILoop]).sprite = "session_panel_status_bg-bottom";
						break;
						default:
						break;
					}
				break;
			}			
		}
	}
	else
	{
		GUIObjectHide("session_gui");
		GUIObjectHide("always_on");
	}

	writeConsole("GUI flipped to " + GUIType + ".");
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
