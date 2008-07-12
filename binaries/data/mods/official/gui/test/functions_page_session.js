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

	// Define categories of cell groups by checking their reference files in the same locations as the icon sheets.
	addCellGroupCategory ("art/textures/ui/session/icons/sheets/");
	// (Note that we don't use this and we probably shouldn't, since the entities state which icon cell they should use; it makes it easier to remember which icon is used
	// for which unit if we have this reference sheet, though.)
	addCellGroupCategory ("art/textures/ui/session/portraits/sheets/");		
}

// ====================================================================

function addCellGroupCategory(iconSheetPath)
{
	// Get array list of all icon sheet reference files.
	var iconSheets = buildDirEntList (iconSheetPath, "*.txt", true);
	// Alphabetically sort the array.
	iconSheets.sort();
	
	// Seek through all icon sheets.
	for (var sheet = 0; sheet < iconSheets.length; sheet++)
	{
		// Get the current icon sheet name.
		var groupName = iconSheets[sheet];
		// Remove path and extension information so we just have the group name.
		groupName = groupName.replace (iconSheetPath, "");
		groupName = groupName.replace (".txt", "");			
		groupName = toTitleCase(groupName);
		
		// Get the elements from the current icon sheet.
		var iconArray = readFileLines (iconSheets[sheet]);
		
		// For each row in the icon sheet file,
		for (var row = 0; row < iconArray.length; row++)
		{
			// Get the individual fields in the array as another array.
			var iconElements = iconArray[row].split (",");
			// Add this cell to the current group.
			addCell (groupName, iconElements[0], iconElements[1]);
		}
	}
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
	for (var i = 0; i <= Crd.last; i++)
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
		
		// Refresh resources
		refreshResources();
	}

	if ( selectionChanged && getGUIObjectByName( "mn" ).hidden == false)
	{	
		// Update manual if it's open.
		refreshManual();
	}
}

// ====================================================================


function confirmLeave()
{
	if (isGameRunning()) 
	{
		endSession("return");
	}
	else
	{
		exit();
	}
}