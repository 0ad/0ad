/*
	DESCRIPTION	: Functions for "main game" session GUI.
	NOTES		: 
*/

// ====================================================================

function initSession()
{
	// ============================================= CONSTANTS =================================================

	snConst = new Object();

	// Portraits (large and small).
	snConst.Portrait = new Object();
	snConst.Portrait.Sml = new Object();
	snConst.Portrait.Sml.Width = 45;
	snConst.Portrait.Sml.Height = snConst.Portrait.Sml.Width;
	snConst.Portrait.Lrg = new Object();
	snConst.Portrait.Lrg.Width = 64;
	snConst.Portrait.Lrg.Height = snConst.Portrait.Lrg.Width;

	// Small icons (eg Movement Rate, Food).
	snConst.MiniIcon = new Object();
	snConst.MiniIcon.Width = 30;
	snConst.MiniIcon.Height = snConst.MiniIcon.Width;

	// ============================================= GLOBALS =================================================

	// Define cell reference constants for icon sheets.
	initCellReference();
}

// ====================================================================

function initCellReference()
{
		cellGroup = new Array();
	
		// Define the cells in this icon sheet.
		groupName = "Armour";
		addCell (groupName, "rating", "Rating");
		addCell (groupName, "hack", "Hack");		
		addCell (groupName, "pierce", "Pierce");		
		addCell (groupName, "crush", "Crush");		

		// Define the cells in this icon sheet.
		groupName = "Attack";
		addCell (groupName, "rating", "Rating");
		addCell (groupName, "hack", "Hack");		
		addCell (groupName, "pierce", "Pierce");		
		addCell (groupName, "crush", "Crush");		
		
		// Define the cells in this icon sheet.
		groupName = "Command";
		addCell (groupName, "patrol", "Patrol");
		addCell (groupName, "townbell", "Town Bell");		
		addCell (groupName, "lock", "Lock");		
		addCell (groupName, "unlock", "Unlock");		
		addCell (groupName, "kill", "Kill");		
		addCell (groupName, "explore", "Explore");		
		addCell (groupName, "rally", "Rally");		
		addCell (groupName, "stop", "Stop");		
		addCell (groupName, "heal", "Heal");		
		addCell (groupName, "repair", "Repair");		
		addCell (groupName, "herd", "Herd");		
		addCell (groupName, "escort", "Escort");		
		addCell (groupName, "move", "Move");		
		addCell (groupName, "attack", "Attack");		
		addCell (groupName, "build", "Build");
		addCell (groupName, "retreat", "Retreat");		

		// Define the cells in this icon sheet.
		groupName = "Find";
		addCell (groupName, "citizen", "Citizen");
		addCell (groupName, "military", "Military");		
		addCell (groupName, "hero", "Hero");				
		addCell (groupName, "civcentre", "Civic Centre");				
		
		// Define the cells in this icon sheet.
		groupName = "Formation";
		addCell (groupName, "box", "Box");
		addCell (groupName, "column_c", "Column Closed");		
		addCell (groupName, "line_c", "Line Closed");				
		addCell (groupName, "column_o", "Column Open");		
		addCell (groupName, "line_o", "Line Open");		
		addCell (groupName, "flank", "Flank");				
		addCell (groupName, "skirmish", "Skirmish");		
		addCell (groupName, "wedge", "Wedge");		
		addCell (groupName, "testudo", "Testudo");		
		addCell (groupName, "phalanx", "Phalanx");	

		// Define the cells in this icon sheet.
		groupName = "Garrison";
		addCell (groupName, "garrison", "Garrison");
		addCell (groupName, "unload", "Unload");		
		addCell (groupName, "unloadtotarget", "Unload to Target");		
		
		// Define the cells in this icon sheet.
		groupName = "Gather";		
		addCell (groupName, "food", "Food");		
		addCell (groupName, "wood", "Wood");				
		addCell (groupName, "stone", "Stone");				
		addCell (groupName, "ore", "Ore");				
		addCell (groupName, "fish", "Fish");				
		addCell (groupName, "fruit", "Fruit");				
		addCell (groupName, "grain", "Grain");				
		addCell (groupName, "meat", "Meat");				
		addCell (groupName, "milk", "Milk");				
		
		// Define the cells in this icon sheet.
		groupName = "Menu";		
		addCell (groupName, "game", "Game");		
		addCell (groupName, "diplomacy", "Diplomacy");				
		addCell (groupName, "objectives", "Objectives");				
		addCell (groupName, "score", "Score");				
		addCell (groupName, "chat", "Chat");				

		// Define the cells in this icon sheet.
		groupName = "MiniMap";		
		addCell (groupName, "flare", "Flare");				
		addCell (groupName, "flare", "Terrain");						
		addCell (groupName, "flare", "Territories");						
		addCell (groupName, "friendorfoe", "Friend or Foe");						
		addCell (groupName, "economic", "Economic");						
		addCell (groupName, "military", "Military");						
		addCell (groupName, "resources", "Resources");						
		
		// Define the cells in this icon sheet.
		groupName = "Rank";		
		addCell (groupName, "advanced", "Advanced");		
		addCell (groupName, "elite", "Elite");		
		
		// Define the cells in this icon sheet.
		groupName = "Replay";		
		addCell (groupName, "pause", "Pause");		
		addCell (groupName, "play", "Play");		
		addCell (groupName, "rewind", "Rewind");		
		addCell (groupName, "fastforward", "Fast Forward");		
		addCell (groupName, "start", "Start");		
		addCell (groupName, "end", "End");		
		addCell (groupName, "cycle", "Cycle");		
		
		// Define the cells in this icon sheet.
		groupName = "Resource";		
		addCell (groupName, "food", "Food");		
		addCell (groupName, "wood", "Wood");		
		addCell (groupName, "stone", "Stone");		
		addCell (groupName, "ore", "Ore");		
		addCell (groupName, "population", "Population");		
		
		// Define the cells in this icon sheet.
		groupName = "Stance";	
		addCell (groupName, "aggress", "Aggress");		
		addCell (groupName, "defend", "Defend");		
		addCell (groupName, "avoid", "Avoid");		
		addCell (groupName, "stand", "Stand");		
		addCell (groupName, "hold", "Hold");		
		
		// Define the cells in this icon sheet.
		groupName = "Statistic";		
		addCell (groupName, "accuracy", "Accuracy");
		addCell (groupName, "vision", "Vision");		
		addCell (groupName, "speed", "Speed");		
		addCell (groupName, "range", "Range");		
		addCell (groupName, "capacity", "Capacity");		
		addCell (groupName, "health", "Health");		
		addCell (groupName, "stamina", "Stamina");		
		
		// Define the cells in this icon sheet.
		groupName = "Tab";		
		addCell (groupName, "structciv", "Construct Civic Buildings");		
		addCell (groupName, "structmil", "Construct Military Buildings");		
		addCell (groupName, "train", "Train");		
		addCell (groupName, "research", "Research");
		addCell (groupName, "barter", "Barter");
		addCell (groupName, "allegiance", "Allegiance");
		addCell (groupName, "selection", "Selection");		
		addCell (groupName, "garrison", "Garrison");		
		addCell (groupName, "command", "Command");		
		
		// Define the cells in this icon sheet.
		groupName = "Civ";
		addCell (groupName, "infantry_swordsman_b", "Basic Infantry Swordsman");
		addCell (groupName, "infantry_swordsman_a", "Advanced Infantry Swordsman");
		addCell (groupName, "infantry_swordsman_e", "Elite Infantry Swordsman");
		addCell (groupName, "infantry_spearman_b", "Basic Infantry Spearman");
		addCell (groupName, "infantry_spearman_a", "Advanced Infantry Spearman");		
		addCell (groupName, "infantry_spearman_e", "Elite Infantry Spearman");				
		addCell (groupName, "infantry_javelinist_b", "Basic Infantry Javelinist");
		addCell (groupName, "infantry_javelinist_a", "Advanced Infantry Javelinist");		
		addCell (groupName, "infantry_javelinist_e", "Elite Infantry Javelinist");
		addCell (groupName, "infantry_archer_b", "Basic Infantry Archer");
		addCell (groupName, "infantry_archer_a", "Advanced Infantry Archer");		
		addCell (groupName, "infantry_archer_e", "Elite Infantry Archer");		
		addCell (groupName, "infantry_slinger_b", "Basic Infantry Slinger");
		addCell (groupName, "infantry_slinger_a", "Advanced Infantry Slinger");		
		addCell (groupName, "infantry_slinger_e", "Elite Infantry Slinger");		
		addCell (groupName, "cavalry_swordsman_b", "Basic Cavalry Swordsman");
		addCell (groupName, "cavalry_swordsman_a", "Advanced Cavalry Swordsman");
		addCell (groupName, "cavalry_swordsman_e", "Elite Cavalry Swordsman");
		addCell (groupName, "cavalry_spearman_b", "Basic Cavalry Spearman");
		addCell (groupName, "cavalry_spearman_a", "Advanced Cavalry Spearman");		
		addCell (groupName, "cavalry_spearman_e", "Elite Cavalry Spearman");				
		addCell (groupName, "cavalry_javelinist_b", "Basic Cavalry Javelinist");
		addCell (groupName, "cavalry_javelinist_a", "Advanced Infantry Javelinist");		
		addCell (groupName, "cavalry_javelinist_e", "Elite Infantry Javelinist");
		addCell (groupName, "cavalry_archer_b", "Basic Infantry Archer");
		addCell (groupName, "cavalry_archer_a", "Advanced Infantry Archer");		
		addCell (groupName, "cavalry_archer_e", "Elite Infantry Archer");		
		addCell (groupName, "super_siege", "Super Siege");
		addCell (groupName, "support_female_citizen", "Female Citizen");		
		addCell (groupName, "support_healer", "Healer");				
		addCell (groupName, "support_trader", "Trader");		
		addCell (groupName, "siege_onager", "Onager");		
		addCell (groupName, "siege_ram", "Ram");				
		addCell (groupName, "siege_ballista", "Ballista");				
		addCell (groupName, "ship_merchant", "Merchant Ship");				
		addCell (groupName, "ship_bireme", "Light Warship");		
		addCell (groupName, "ship_trireme", "Medium Warship");		
		addCell (groupName, "ship_quinquereme", "Heavy Warship");		
		addCell (groupName, "super_infantry", "Super Infantry");		
		addCell (groupName, "super_cavalry", "Super Cavalry");		
		addCell (groupName, "hero_1", "Hero");		
		addCell (groupName, "hero_2", "Hero");				
		addCell (groupName, "hero_3", "Hero");			
		addCell (groupName, "civil_centre", "Civic Centre");			
		addCell (groupName, "house", "House");		
		addCell (groupName, "farmstead", "Farmstead");		
		addCell (groupName, "field", "Field");		
		addCell (groupName, "corral", "Corral");		
		addCell (groupName, "mill", "Mill");		
		addCell (groupName, "scout_tower", "Outpost");		
		addCell (groupName, "wall", "Wall");		
		addCell (groupName, "wall_tower", "Wall Tower");		
		addCell (groupName, "wall_gate", "Wall Gate");		
		addCell (groupName, "dock", "Dock");		
		addCell (groupName, "temple", "Temple");		
		addCell (groupName, "barracks", "Barracks");		
		addCell (groupName, "market", "Market");		
		addCell (groupName, "fortress", "Fortress");		
		addCell (groupName, "sb1", "Special Building");		
		addCell (groupName, "sb2", "Special Building");		
}

// ====================================================================

function addCell (group, cellID, cellName)
{
	// Add a cell ID to a cell container group.

	// If this array does not exist,
	if (!cellGroup[group])
	{
		// Create the container.
		cellGroup[group] = new Array();	
		cellGroup[group].length = new Object(0);
	}

	cellGroup[group][cellID]		= new Array();
	cellGroup[group][cellID].id		= new Object (cellGroup[group].length);
	cellGroup[group][cellID].name	= new Object (cellName);
	
	cellGroup[group].length++;
}

// ====================================================================

function setPortrait(objectName, portraitString, portraitSuffix, portraitCell) 
{
        // Use this function as a shortcut to change a portrait object to a different portrait image. 

        // Accepts an object and specifies its default, rollover (lit) and disabled (gray) sprites.
        // Sprite Format: "sn""portraitString""portraitSuffix"
        // Sprite Format: "sn""portraitString""portraitSuffix""Over"
        // Sprite Format: "sn""portraitString""portraitSuffix""Disabled"
        // Note: Make sure the file follows this naming convention or bad things could happen.

        // Get GUI object
        setPortraitGUIObject = getGUIObjectByName(objectName);

	// Report error if object not found.
	if (!setPortraitGUIObject)
	{
		console.write ("setPortrait(): Failed to find object " + objectName + ".");
		return 1;
	}

        // Set the three portraits.
	if (portraitSuffix && portraitSuffix != "")
	        setPortraitGUIObject.sprite = "sn" + portraitString + portraitSuffix;
	else
	        setPortraitGUIObject.sprite = "sn" + portraitString;

        setPortraitGUIObject.sprite_over = setPortraitGUIObject.sprite + "Over";
        setPortraitGUIObject.sprite_disabled = setPortraitGUIObject.sprite + "Disabled";

        // If the source texture is a multi-frame image (icon sheet), specify correct cell.
        if (portraitCell && portraitCell != "")
                setPortraitGUIObject.cell_id = portraitCell;
	else
		setPortraitGUIObject.cell_id = "";

	return 0;
}

// ====================================================================

function flipGUI (NewGUIType)
{
	// Changes GUI to a different layout.

	switch (NewGUIType)
	{
		// Set which GUI to use.
		case rb:
		case lb:
		case lt:
		case rt:
			GUIType = NewGUIType;
		break;
		default:
			// If no type specified, toggle.
			if (GUIType == rb)
				GUIType = lb;
			else
			if (GUIType == lb)
				GUIType = lt;
			else
			if (GUIType == lt)
				GUIType = rt;
			else
				GUIType = rb;
		break;
	}
	// Seek through all sizes created.
	for (i = 0; i <= Crd.last; i++)
	{
		// Set their sizes to the stored value.
		getGUIObjectByName (Crd[i].name).size = Crd[i].size[GUIType];
	}

	writeConsole("GUI flipped to size " + GUIType + ".");
}

// ====================================================================

// Update-on-alteration trickery...

// We don't really want to update every single time we get a
// selection-changed or property-changed event; that could happen
// a lot. Instead, use this bunch of globals to cache any changes
// that happened between GUI updates.

// This boolean determines whether the selection has been changed.
var selectionChanged = false;

// This boolean determines what the template of the selected object
// was when last we looked
var selectionTemplate = null;

// This array holds the name of all properties that need to be updated
var selectionPropertiesChanged = new Array();

// This array holds a list of all the objects we hold property-change 
// watches on
var propertyWatches = new Array();

// ====================================================================
 
// This function resets all the update variables, above

function resetUpdateVars()
{
	if( selectionChanged )
	{
		for( watchedObject in propertyWatches )
			propertyWatches[watchedObject].unwatchAll( selectionWatchHandler ); // Remove the handler
		
		propertyWatches = new Array();
		if( selection.length > 0 && selection[0] )
		{
			// Watch the object itself
			selection[0].watchAll( selectionWatchHandler );
			propertyWatches.push( selection[0] );
			// And every parent back up the tree (changes there will affect
			// displayed properties via inheritance)
			var parent = selection[0].template
			while( parent )
			{
				parent.watchAll( selectionWatchHandler );
				propertyWatches.push( selection[0] );
				parent = parent.parent;
			}
		}
	}
	selectionChanged = false;
	if( selection.length > 0 && selection[0] ) 
	{
		selectionTemplate = selection[0].template;
	}
	else
		selectionTemplate = null;
		
	selectionPropertiesChanged = new Array();
}

// ====================================================================

// This function returns whether we should update a particular statistic
// in the GUI (e.g. "actions.attack") - this should happen if: the selection
// changed, the selection had its template altered (changing lots of stuff)
// or an assignment has been made to that stat or any property within that
// stat. 

function shouldUpdateStat( statname )
{
	if( selectionChanged || ( selectionTemplate != selection[0].template ) )
	{
		return( true );
	}
	for( var property in selectionPropertiesChanged )
	{
		// If property starts with statname
		if( selectionPropertiesChanged[property].substring( 0, statname.length ) == statname )
		{
			return( true );
		}
	}
	return( false );
}

// ====================================================================

// This function is a handler for the 'selectionChanged' event,
// it updates the selectionChanged flag

function selectionChangedHandler()
{
	selectionChanged = true;
}

// ====================================================================

// Register it.
addGlobalHandler( "selectionChanged", selectionChangedHandler );

// ====================================================================

// This function is a handler for a watch event; it updates the
// selectionPropertiesChanged array.

function selectionWatchHandler( propname, oldvalue, newvalue )
{
	selectionPropertiesChanged.push( propname );
	// This bit's important (watches allow the handler to change the value
	// before it gets written; we don't want to affect things, so make sure
	// the value we send back is the one that was going to be written)
	return( newvalue ); 
}

// ====================================================================

function snRefresh() 
{
	// Updated each tick to refresh session controls if necessary.

	// Don't process GUI when we're full-screen.
	if (getGUIObjectByName ("sn").hidden == false)
	{
		if (!selection.length)         // If no entity selected,
		{
			// Hide Status Orb
			guiHide ("snStatusPane");

			// Hide Group Pane.
//			guiHide ("snGroupPane");

			getGlobal().MultipleEntitiesSelected = 0;
		}
		else                        // If at least one entity selected,
		{
			// Reveal Status Orb
			guiUnHide ("snStatusPane");
			
// (later, we need to base this on the selected unit's stats changing)
			refreshStatusPane(); 

			// Check if a group of entities selected
			if (selection.length > 1) 
			{
				// If a group pane isn't already open, and we don't have the same set as last time,
				if (selection.length != getGlobal().MultipleEntitiesSelected)
				{
// (later, we need to base this on the selection changing)
//					refreshGroupPane(); 
					getGlobal().MultipleEntitiesSelected = selection.length;
				}
			} 
			else
			{
				getGlobal().MultipleEntitiesSelected = 0;

				// Hide Group Pane.
//				guiHide ("snGroupPane");
			}
		}

		// Modify any resources given/taken
// (later, we need to base this on a resource-changing event).
//		refreshResourcePool();

		// Update Team Tray
// (later, we need to base this on the player creating a group).
//		refreshTeamTray();
		
		// Refresh handler that monitors whether property/selection has been updated.
		resetUpdateVars();
	}

	if ( selectionChanged && getGUIObjectByName( "mn" ).hidden == false)
	{	
		// Update manual if it's open.
		refreshManual();
	}
}

// ====================================================================