function initSession()
{
        initGroupPane();
        initResourcePool();
        initStatusOrb();
        initMapOrb();
        initTeamTray();
        initSubWindows();
}

// ====================================================================

function initCellReference()
{
        // Define cell reference constants for icon sheets.

        // icon_sheet_statistic
        stat_accuracy                         = 0;
        stat_attack                         = 1;
        stat_armour                         = 2;
        stat_los                         = 3;
        stat_speed                         = 4;
        stat_range                         = 5;
        stat_hack                         = 6;
        stat_pierce                         = 7;
        stat_crush                         = 8;
        stat_rank1                         = 9;
        stat_rank2                         = 10;
        stat_rank3                         = 11;
        stat_garrison                         = 12;
        stat_heart                         = 13;

        // portrait_sheet_action
                // generic actions
        action_empty                        = 0;
        action_attack                        = 1;
        action_patrol                        = 2;
        action_stop                        = 3;
        action_gather_food                = 4;
        action_gather_wood                = 5;
        action_gather_stone                = 6;
        action_gather_ore                = 7;
        action_rally                        = 8;
        action_repair                        = 9;
        action_heal                        = 10;
        action_scout                        = 11;
        action_townbell                        = 12;
        action_lock                        = 13;
        action_unlock                        = 14;
                // formation actions
        action_formation_box                = 23;
        action_formation_column_c        = 24;
        action_formation_column_o        = 25;
        action_formation_line_c                = 26;
        action_formation_line_o                = 27;
        action_formation_phalanx        = 28;
        action_formation_skirmish        = 29;
        action_formation_testudo        = 30;
        action_formation_wedge                = 31;
                // stance actions
        action_stance_aggress                = 39;
        action_stance_avoid                = 40;
        action_stance_defend                = 41;
        action_stance_hold                = 42;
        action_stance_stand                = 43;
                // tab actions
        action_tab_command                = 48;
        action_tab_train                = 49;
        action_tab_buildciv                = 50;
        action_tab_buildmil                = 51;
        action_tab_research                = 52;
        action_tab_formation                = 53;
        action_tab_stance                = 54;
        action_tab_barter                = 55;
}

// ====================================================================

function startLoadingScreen()
{
        // Setup loading screen.

        // Switch screens from main menu to loading screen.
        GUIObjectHide("pregame_gui");
        GUIObjectUnhide("loading_screen");
        console.write("Loading " + g_GameAttributes.mapFile + " (" + g_GameAttributes.numPlayers + " players) ...");

        getGUIObjectByName("loading_screen_titlebar_text").caption = "Loading Scenario ...";
        getGUIObjectByName("loading_screen_progress_bar_text").caption = "... Reticulating splines ...";
        getGUIObjectByName("loading_screen_progress_bar").caption = 80;
        getGUIObjectByName("loading_screen_text").caption = "LOADING " + g_GameAttributes.mapFile + " ...\nPlease wait ...\n(Yes, we know the progress bar doesn't do diddly squat right now)\nJust keep waiting ...\nIt'll get there ...\nAlmost done ...\nTrust me!";
        getGUIObjectByName("loading_screen_tip").caption = "Wise man once say ...\nHe who thinks slow, he act in haste, be rash and quick and foolish. But he that thinks too much, acts too slowly. The stupid always win, Commandersan. Remember that. You are tiny grasshopper.";

        // Begin game session.
        setTimeout( loadSession, 20 );
}

// ====================================================================

function loadSession()
{
        startGame();
        GUIObjectHide("loading_screen");
        GUIObjectUnhide("session_gui");
        FlipGUI(GUIType);

        // Select session peace track.
        curr_session_playlist_1 = newRandomSound("music", "peace");
        // Fade out main theme and fade in session theme.
        CrossFade(curr_music, curr_session_playlist_1, 0.0001);
}

// ====================================================================

function setPortrait(objectName, portraitString, portraitCell) 
{
        // Use this function as a shortcut to change a portrait object to a different portrait image. 

        // Accepts an object and specifies its default, rollover (lit) and disabled (grey) sprites.
        // Sprite Format: "ui_portrait_"portraitString
        // Sprite Format: "ui_portrait_"portraitString"_lit"
        // Sprite Format: "ui_portrait_"portraitString"_grey"
        // Note: Make sure the file follows this naming convention or bad things could happen.

        // Get GUI object
        GUIObject = getGUIObjectByName(objectName);

        // Set the three portraits.
        GUIObject.sprite = "ui_portrait_" + portraitString;
        // Note we need to use a special syntax here (object["param"] instead of object.param because dashes aren't actually in JS's variable-naming conventions.
        GUIObject.sprite_over = GUIObject.sprite + "_lit";
        GUIObject.sprite_disabled = GUIObject.sprite + "_grey";

        // If the source texture is a multi-frame image (icon sheet), specify correct cell.
        if (portraitCell)
                GUIObject.cell_id = portraitCell;
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
