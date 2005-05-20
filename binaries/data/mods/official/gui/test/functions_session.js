function initSession()
{
	// ============================================= CONSTANTS =================================================

	GUIType="bottom";

	// Coord-style size table.
	SizeCoord = new Array();
	SizeCoord.last = 0;

	// Standard portrait widths.
	crd_portrait_lrg_width = 64;
	crd_portrait_lrg_height = crd_portrait_lrg_width;
	crd_portrait_sml_width = 32;
	crd_portrait_sml_height = crd_portrait_sml_width;

	// Small icons (eg Movement Rate, Food).
	crd_mini_icon_width = 20;
	crd_mini_icon_height = crd_mini_icon_width;

	// Define cell reference constants for icon sheets.
	initCellReference();

	// ============================================= GLOBALS =================================================

        initGroupPane();
        initResourcePool();
        initStatusOrb();
        initMapOrb();
        initTeamTray();
        initSubWindows();
		initManual();
		initJukebox();
}

// ====================================================================

function initCellReference()
{
        // Define cell reference constants for icon sheets.

        // icon_sheet_statistic
        stat_accuracy                   = 0;
        stat_attack                     = 1;
        stat_armour                     = 2;
        stat_los                        = 3;
        stat_speed                      = 4;
        stat_range                      = 5;
        stat_hack                       = 6;
        stat_pierce                     = 7;
        stat_crush                      = 8;
        stat_rank1                      = 9;
        stat_rank2                      = 10;
        stat_rank3                      = 11;
        stat_garrison                   = 12;
        stat_heart                      = 13;

        // portrait_sheet_action
                // generic actions
        action_empty                    = 0;
        action_attack                   = 1;
        action_patrol                   = 2;
        action_stop                     = 3;
        action_gather_food              = 4;
        action_gather_wood              = 5;
        action_gather_stone             = 6;
        action_gather_ore               = 7;
        action_rally                    = 8;
        action_repair                   = 9;
        action_heal                     = 10;
        action_scout                    = 11;
        action_townbell                 = 12;
        action_lock                     = 13;
        action_unlock                   = 14;
                // formation actions
        action_formation_box            = 23;
        action_formation_column_c       = 24;
        action_formation_column_o       = 25;
        action_formation_line_c         = 26;
        action_formation_line_o         = 27;
        action_formation_phalanx        = 28;
        action_formation_skirmish       = 29;
        action_formation_testudo        = 30;
        action_formation_wedge          = 31;
                // stance actions
        action_stance_aggress           = 39;
        action_stance_avoid             = 40;
        action_stance_defend            = 41;
        action_stance_hold              = 42;
        action_stance_stand             = 43;
                // tab actions
        action_tab_command              = 48;
        action_tab_train                = 49;
        action_tab_buildciv             = 50;
        action_tab_buildmil             = 51;
        action_tab_research             = 52;
        action_tab_formation            = 53;
        action_tab_stance               = 54;
        action_tab_barter               = 55;
}

// ====================================================================

function setPortrait(objectName, portraitString, portraitSuffix, portraitCell) 
{
        // Use this function as a shortcut to change a portrait object to a different portrait image. 

        // Accepts an object and specifies its default, rollover (lit) and disabled (grey) sprites.
        // Sprite Format: "ui_portrait_"portraitString"_"portraitSuffix
        // Sprite Format: "ui_portrait_"portraitString"_"portraitSuffix"_lit"
        // Sprite Format: "ui_portrait_"portraitString"_"portraitSuffix"_grey"
        // Note: Make sure the file follows this naming convention or bad things could happen.

        // Get GUI object
        setPortraitGUIObject = getGUIObjectByName(objectName);

        // Set the three portraits.
	if (portraitSuffix && portraitSuffix != "")
	        setPortraitGUIObject.sprite = "ui_portrait_" + portraitString + "_" + portraitSuffix;
	else
	        setPortraitGUIObject.sprite = "ui_portrait_" + portraitString;

        setPortraitGUIObject.sprite_over = setPortraitGUIObject.sprite + "_lit";
        setPortraitGUIObject.sprite_disabled = setPortraitGUIObject.sprite + "_grey";

        // If the source texture is a multi-frame image (icon sheet), specify correct cell.
        if (portraitCell && portraitCell != "")
                setPortraitGUIObject.cell_id = portraitCell;
	else
		setPortraitGUIObject.cell_id = "";
}

// ====================================================================

function getObjectInfo() 
{
        // Updated each tick to extract entity information from selected unit(s).

        // Don't process GUI when we're full-screen.
        if (GUIType != "none")
        {
                if (!selection.length)         // If no entity selected,
                {
                        // Hide Status Orb
                        getGUIObjectByName("session_status_orb").hidden = true;

                        // Hide Group Pane.
                        getGUIObjectByName("session_group_pane").hidden = true;

                        getGlobal().MultipleEntitiesSelected = 0;
                }
                else                        // If at least one entity selected,
                {
                        // Store globals for entity information.
//                        strString = "" + selection[0].position;
//                        EntityPos = strString.substring(20,strString.length-3);

                        UpdateStatusOrb(); // (later, we need to base this on the selected unit's stats changing)

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
                                        UpdateGroupPane(); // (later, we need to base this on the selection changing)
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

                // Modify any resources given/taken (later, we need to base this on a resource-changing event).
                UpdateResourcePool();

                // Update Team Tray (later, we need to base this on the player creating a group).
                UpdateTeamTray();
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
		GUIObjectUnhide("SESSION_GUI");
		GUIObjectUnhide("always_on");

		// Seek through all sizes created.
		for (FlipGUILoop = 0; FlipGUILoop <= SizeCoord.last-1; FlipGUILoop++)
		{
			// Set each object to the other size.
			switch (GUIType)
			{
				case "bottom":
					setSize(SizeCoord[FlipGUILoop].name, SizeCoord[FlipGUILoop].size1);
					switch (SizeCoord[FlipGUILoop].name){
						case "SN_MAP_ORB_SEGBOTTOM1":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop1";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segtop1" + "_lit";
						break;
						case "SN_MAP_ORB_SEGBOTTOM2":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop2";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segtop2" + "_lit";
						break;
						case "SN_MAP_ORB_SEGBOTTOM3":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop3";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segtop3" + "_lit";
						break;
						case "SN_MAP_ORB_SEGBOTTOM4":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop4";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segtop4" + "_lit";
						break;
						case "SN_STATUS_PANE_BG":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_status_bg_top";
						break;
						default:
						break;
					}
				break;
				case "top":
					setSize(SizeCoord[FlipGUILoop].name, SizeCoord[FlipGUILoop].size2);
					switch (SizeCoord[FlipGUILoop].name){
						case "SN_MAP_ORB_SEGBOTTOM1":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segbottom1";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segbottom1_lit";
						break;
						case "SN_MAP_ORB_SEGBOTTOM2":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segbottom2";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segbottom2_lit";
						break;
						case "SN_MAP_ORB_SEGBOTTOM3":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segbottom3";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segbottom3_lit";
						break;
						case "SN_MAP_ORB_SEGBOTTOM4":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segbottom4";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segbottom4_lit";
						break;
						case "SN_STATUS_PANE_BG":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_status_bg_bottom";
						break;
						default:
						break;
					}
				break;
			}			
		}

		UpdateGroupPane();
	}
	else
	{
		GUIObjectHide("SESSION_GUI");
		GUIObjectHide("always_on");
	}

	writeConsole("GUI flipped to " + GUIType + ".");

}

// ====================================================================

// Unpleasant system-dependent hack. The input system should be fixed...
var SDL_BUTTON_LEFT = 0;
var SDL_BUTTON_RIGHT = 1;
var SDL_BUTTON_MIDDLE = 2;
var SDL_BUTTON_WHEELUP = 3;
var SDL_BUTTON_WHEELDOWN = 4;

function selectEntity(handler)
{
	endSelection();
	startSelection(function (event) {
			// Selection is performed when single-clicking the right mouse
			// button.
			if (event.button == SDL_BUTTON_RIGHT && event.clicks == 1)
			{
				handler(event.entity);
			}
			// End selection on first mouse-click
			endSelection();
		});
}

function selectLocation(handler)
{
	endSelection();
	startSelection(function (event) {
			// Selection is performed when single-clicking the right mouse
			// button.
			if (event.button == SDL_BUTTON_RIGHT && event.clicks == 1)
			{
				handler(event.x, event.y);
			}
			// End selection on first mouse-click
			endSelection();
		});
}

function startSelection(handler)
{
	gameView.startCustomSelection();
	getGlobal().selectionWorldClickHandler=handler;
	console.write("isSelecting(): "+isSelecting());
}

function endSelection()
{
	if (!isSelecting())
		return;
	
	gameView.endCustomSelection();
	getGlobal().selectionWorldClickHandler = null;
}

function isSelecting()
{
	return getGlobal().selectionWorldClickHandler != null;
}

// The world-click handler - called whenever the user clicks the terrain
function worldClickHandler(event)
{
	args=new Array(null, null);

	console.write("worldClickHandler: button "+event.button+", clicks "+event.clicks);

	if (isSelecting())
	{
		getGlobal().selectionWorldClickHandler(event);
		return;
	}


	// Right button single- or double-clicks
	if (event.button == SDL_BUTTON_RIGHT && event.clicks <= 2)
	{
		if (event.clicks == 1)
			cmd = event.command;
		else if (event.clicks == 2)
		{
			console.write("Issuing secondary command");
			cmd = event.secondaryCommand;
		}
	}
	else
		return;

	switch (cmd)
	{
		// location target commands
		case NMT_Goto:
		case NMT_Patrol:
			if (event.queued)
			{
				cmd = NMT_AddWaypoint;
			}
		case NMT_AddWaypoint:
			args[0]=event.x;
			args[1]=event.y;
			break;
		// entity target commands
		case NMT_AttackMelee:
		case NMT_Gather:
			args[0]=event.entity;
			args[1]=null;
			break;
		default:
			console.write("worldClickHandler: Unknown command: "+cmd);
			return;
	}

	issueCommand(selection, cmd, args[0], args[1]);
}

addGlobalHandler("worldClick", worldClickHandler);
