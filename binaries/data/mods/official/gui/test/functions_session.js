function initSession()
{
	GUIType="bottom";

	// Coord-style size table.
	SizeCoord = new Array();
	SizeCoord.last = 0;

	// Standard portrait widths.
	crd_portrait_lrg_width = 64;
	crd_portrait_lrg_height = 64;
	crd_portrait_sml_width = 32;
	crd_portrait_sml_height = 32;

	initGroupPane();
}

// ====================================================================

function AddSizeCoord(objectName, left1, top1, right1, bottom1, rleft1, rtop1, rright1, rbottom1, left2, top2, right2, bottom2, rleft2, rtop2, rright2, rbottom2)
{
	// Used to store the two GUI style sizes for an object on creation (specified as coordinates).
	// Used later by FlipGUI() to switch the objects to a new set of positions.

	SizeCoord[SizeCoord.last] = new Object();
	SizeCoord[SizeCoord.last].name = objectName;
	SizeCoord[SizeCoord.last].size1 = new GUISize(left1, top1, right1, bottom1, rleft1, rtop1, rright1, rbottom1);
	SizeCoord[SizeCoord.last].size2 = new GUISize(left2, top2, right2, bottom2, rleft2, rtop2, rright2, rbottom2);

	SizeCoord.last++; // Increment counter for next entry.
}

// ====================================================================

function AddSizeString(objectName, size1, size2)
{
	// Used to store the two GUI style sizes for an object on creation (specified as strings).
	// Used later by FlipGUI() to switch the objects to a new set of positions.

	SizeCoord[SizeCoord.last] = new Object();
	SizeCoord[SizeCoord.last].name = objectName;
	SizeCoord[SizeCoord.last].size1 = size1;
	SizeCoord[SizeCoord.last].size2 = size2;

	SizeCoord.last++; // Increment counter for next entry.
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
	// Use this function as a shortcut to change the size of a GUI control, specifying a size string. 

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

		getGlobal().MultipleEntitiesSelected = 0;
	}
	else			// If at least one entity selected,
	{
		// Store globals for entity information.
//		strString = "" + selection[0].position;
//		EntityPos = strString.substring(20,strString.length-3);


		// Update name text.
		// Personal name.
		if (selection[0].traits.id.personal && selection[0].traits.id.personal != "")
		{
			GUIObject = getGUIObjectByName("session_panel_status_name1");
			GUIObject.caption = selection[0].traits.id.personal + "\n";
		}
		else{
			GUIObject = getGUIObjectByName("session_panel_status_name1");
		}
		// Generic name.
		if (selection[0].traits.id.generic)
		{
			GUIObject = getGUIObjectByName("session_panel_status_name2");
			GUIObject.caption = selection[0].traits.id.generic + "\n";
		}
		else{
			GUIObject = getGUIObjectByName("session_panel_status_name2");
			GUIObject.caption = "";
		}
		// Specific/ranked name.
		if (selection[0].traits.id.ranked)
		{
			GUIObject = getGUIObjectByName("session_panel_status_name3");
			GUIObject.caption = selection[0].traits.id.ranked + "\n";
		}
		else{
			if (selection[0].traits.id.specific)
			{
				GUIObject = getGUIObjectByName("session_panel_status_name3");
				GUIObject.caption = selection[0].traits.id.specific + "\n";
			}
		}

		// Update portrait
		if (selection[0].traits.id.icon)
		{
			if (selection[0].traits.id.icon_cell)
				setPortrait("session_panel_status_portrait", selection[0].traits.id.icon + "_" + selection[0].traits.id.icon_cell);
			else
				setPortrait("session_panel_status_portrait", selection[0].traits.id.icon);
		}

		// Update hitpoints
		if (selection[0].traits.health.curr & selection[0].traits.health.hitpoints)
		{
			getGUIObjectByName("session_panel_status_icon_hp_text").caption = Math.round(selection[0].traits.health.curr) + "/" + Math.round(selection[0].traits.health.hitpoints);
			getGUIObjectByName("session_panel_status_icon_hp_bar").caption = ((Math.round(selection[0].traits.health.curr) * 100 ) / Math.round(selection[0].traits.health.hitpoints));
		}

		// Reveal Status Orb
		getGUIObjectByName("session_status_orb").hidden = false;

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
							setSize("session_group_pane_bg", SizeCoord[FlipGUILoop].size1);
						break;
						case "bottom":
							setSize("session_group_pane_bg", SizeCoord[FlipGUILoop].size2);
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
						// Set progress bar for hitpoints.
						if (selection[groupPaneLoop-1].traits.health.curr && selection[groupPaneLoop-1].traits.health.hitpoints)
							groupPaneBar.caption = ((Math.round(selection[groupPaneLoop-1].traits.health.curr) * 100 ) / Math.round(selection[groupPaneLoop-1].traits.health.hitpoints));
						if (selection[groupPaneLoop-1].traits.id.icon)
							setPortrait("session_group_pane_portrait_" + groupPaneLoop, selection[groupPaneLoop-1].traits.id.icon);
					}
					// If it's empty, hide its group portrait.
					else
					{
						groupPanePortrait.hidden = true;
						groupPaneBar.hidden = true;
					}

				}

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
		for (FlipGUILoop = 0; FlipGUILoop < SizeCoord.last; FlipGUILoop++)
		{
			// Set each object to the other size.
			switch (GUIType)
			{
				case "top":
					setSize(SizeCoord[FlipGUILoop].name, SizeCoord[FlipGUILoop].size1);
					switch (SizeCoord[FlipGUILoop].name){
						case "session_panel_minimap_segbottom1":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = SizeCoord[FlipGUILoop].name;
						break;
						case "session_panel_minimap_segbottom2":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = SizeCoord[FlipGUILoop].name;
						break;
						case "session_panel_minimap_segbottom3":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = SizeCoord[FlipGUILoop].name;
						break;
						case "session_panel_minimap_segbottom4":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = SizeCoord[FlipGUILoop].name;
						break;
						case "session_panel_status_bg":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_status_bg-top";
						break;
						default:
						break;
					}
				break;
				case "bottom":
					setSize(SizeCoord[FlipGUILoop].name, SizeCoord[FlipGUILoop].size2);
					switch (SizeCoord[FlipGUILoop].name){
						case "session_panel_minimap_segbottom1":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop1";
						break;
						case "session_panel_minimap_segbottom2":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop2";
						break;
						case "session_panel_minimap_segbottom3":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop3";
						break;
						case "session_panel_minimap_segbottom4":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop4";
						break;
						case "session_panel_status_bg":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_status_bg-bottom";
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
