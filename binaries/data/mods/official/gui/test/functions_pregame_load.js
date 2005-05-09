// HACK: This replaces the old numPlayers hack to set up a bunch of sample
// players in SP games to show off some player colours and fights
function setupSPPlayers()
{
	// The first player is by default allocated to the local player in SP, so
	// adding 7 players means that we'll have 8 players in total
	for (i=0;i<7;i++)
	{
		g_GameAttributes.slots[i+1].assignLocal();
		console.write("Slot "+(i+1)+" is assigned: "+g_GameAttributes.slots[i+1].assignment);
	}
}

function startLoadingScreen()
{
        // Switch screens from main menu to loading screen.
        GUIObjectHide("PREGAME_GUI");
        GUIObjectUnhide("loading_screen");
        console.write("Loading " + g_GameAttributes.mapFile + " (" + g_GameAttributes.numPlayers + " players) ...");

        // Generate random number for random concept art (should be depending on the selected civ later)
        randvar = getRandom(1, 6);
        
        switch (randvar) {
          case 1:
            sprite = "load_concept_he";
            break;
          case 2:
            sprite = "load_concept_ce";
            break;
          case 3:
            sprite = "load_concept_pe";
            break;
          case 4:
            sprite = "load_concept_ro";
            break;
          case 5:
            sprite = "load_concept_ca";
            break;
          case 6:
            sprite = "load_concept_ib";
            break;
        }
        
        getGUIObjectByName("loading_screen_background_concept").sprite = sprite;
        console.write(getGUIObjectByName("loading_screen_background_concept").sprite);

        // janwas: main loop now sets progress / description, but that won't
        // happen until the first timeslice completes, so set initial values.
        getGUIObjectByName("loading_screen_titlebar_text").caption = "Loading Scenario ...";
        getGUIObjectByName("loading_screen_progress_bar_text").caption = "";
        getGUIObjectByName("loading_screen_progress_bar").caption = 0;
        getGUIObjectByName("loading_screen_text").caption = "LOADING " + g_GameAttributes.mapFile + " ...\nPlease wait ...";

	// Pick a random tip of the day (each line is a separate tip).
	tipArray  = readFileLines("gui/text/tips.txt");
	// Set tip string.
        getGUIObjectByName("loading_screen_tip").caption = tipArray[getRandom(0, tipArray.length-1)];

        // Begin game session.
        if (! startGame())
        {
                // Failed to start the game; go back to the main menu.
                GUIObjectHide("loading_screen");
                GUIObjectUnhide("PREGAME_GUI");
                // Show an error message
                btCaptions = new Array("OK");
                btCode = new Array("");
                messageBox(400, 200, "The game could not be started with the given parameters. You probably have entered an invalid map name.", "Error", 0, btCaptions, btCode);
        }
}

// ====================================================================

function reallyStartGame()
{
        // Create resource pools for each player, etc.
        setupSession();        

        FlipGUI(GUIType);

        // Select session peace track.
        curr_session_playlist_1 = newRandomSound("music", "peace");
        // Fade out main theme and fade in session theme.
        CrossFade(curr_music, curr_session_playlist_1, 0.1);
			// janwas: greatly accelerate this timesink;
			// will be replaced soon by native version that doesn't block.

        // Switch GUI from main menu to game session.
        GUIObjectHide("loading_screen");
        GUIObjectUnhide("SESSION_GUI");
}

// ====================================================================

function setupSession()
{
        // Do essentials that can only be done when the session has been loaded ...
        // For example, create the resource types.
        // Initialise Resource Pools by attaching them to the Player object.
        // (CPlayer code takes care of giving a copy to each player.)
        CreateResources();

        // Start refreshing the session controls.
        setInterval( getObjectInfo, 1, 100 );
}

// ====================================================================

function endSession(closeType)
{
        // Occurs when the player chooses to close the current game.

        switch (closeType)
        {
                case ("return"):
                        // If the player has chosen to quit game and return to main menu,

                        // End the session.
                        endGame();
                        
                        // Fade out current music and return to playing menu theme.
                        curr_music = newRandomSound('music', 'menu');
                        CrossFade(curr_session_playlist_1, curr_music, 0.1);
                                // janwas: greatly accelerate this timesink;
                                // will be replaced soon by native version that doesn't block.

                        // Swap GUIs to display main menu.
                        GUIObjectHide('SESSION_GUI');
                        GUIObjectUnhide('PREGAME_GUI');
                break;
                case ("exit"):
                        // If the player has chosen to shutdown and immediately return to operating system,

                        exit();
                break;
        }
}

// ====================================================================
