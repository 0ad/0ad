/*
	DESCRIPTION	: Functions used load/end the game session and manipulate the loading screen.
	NOTES		: 
*/

// ====================================================================

function startMap (mapName, losSetting, openWindow)
{
	// Starts the map, closing the current window.
	// mapName: 	.pmp to load.
	// openWindow: 	Window group (usually parent string) of control that called the function. It'll be hidden.

	// Check whether we have a correct file extension, to avoid crashes
	extension = mapName.substring (mapName.length, mapName.length-4);
	if (extension != ".pmp")
	{
		// Add .pmp to the file name.
		mapName =  mapName + ".pmp";
	}

	// Set up game
	g_GameAttributes.mapFile = mapName;
	g_GameAttributes.losSetting = losSetting;

	// Close setup window
	closeMainMenuSubWindow (openWindow);

	// Display loading screen.	
	startLoadingScreen();

        // Begin game session.
        if (! startGame())
        {
                // Failed to start the game; go back to the main menu.
                guiSwitch ("ld", "pg");
                // Show an error message
                btCaptions = new Array("OK");
                btCode = new Array("");
                messageBox(400, 200, "The game could not be started with the given parameters. You probably have entered an invalid map name.", "Error", 0, btCaptions, btCode);
        }
}

// ====================================================================

function startLoadingScreen()
{
        // Switch screens from main menu to loading screen.
        guiSwitch ("pg", "ld");
        console.write ("Loading " + g_GameAttributes.mapFile + " (" + g_GameAttributes.numPlayers + " players) ...");

        // Generate random number for random concept art (should be depending on the selected civ later)
        randVar = getRandom(1, 6);
        
	switch (randVar) {
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
        
        getGUIObjectByName ("ldConcept").sprite = sprite;

        // janwas: main loop now sets progress / description, but that won't
        // happen until the first timeslice completes, so set initial values.
        getGUIObjectByName ("ldTitleBar").caption = "Loading Scenario ...";
        getGUIObjectByName ("ldProgressBarText").caption = "";
        getGUIObjectByName ("ldProgressBar").caption = 0;
        getGUIObjectByName ("ldText").caption = "LOADING " + g_GameAttributes.mapFile + " ...\nPlease wait ...";

        // Pick a random tip of the day (each line is a separate tip).
        tipArray  = readFileLines("gui/text/tips.txt");
        // Set tip string.
        getGUIObjectByName ("ldTip").caption = tipArray[getRandom(0, tipArray.length-1)];
}

// ====================================================================

function reallyStartGame()
{
	// This is a reserved function name that is executed by the engine when it is ready
	// to start the game (ie loading progress has reached 100%).

	// Create resource pools for each player, etc.
	setupSession();  

	// Switch GUI from loading screen to game session.
	guiSwitch ("ld", "sn");
}

// ====================================================================

function setupSession ()
{
        // Do essentials that can only be done when the session has been loaded ...
        // For example, create the resource types, scores, etc, for each player.

        // Initialise Resource Pools by attaching them to the Player object.
        // (CPlayer code takes care of giving a copy to each player.)
        createResources();
		
		

/*
	if (sessionType == "Skirmish")
	{
		// Set up a bunch of players so we can see them pretty colours. :P
		console.write ("Setting Up Temporary Single Players");
		// The first player is by default allocated to the local player in SP, so
		// adding 7 players means that we'll have 8 players in total
		for (i=0; i<7; i++)
		{
			g_GameAttributes.slots[i+1].assignLocal();
			console.write ("Slot "+(i+1)+" is assigned: " + g_GameAttributes.slots[i+1].assignment);
		}
	}
*/
	// Set starting UI layout.
	GUIType=rb;
        flipGUI (GUIType);

        // Select session peace track.
        curr_session_playlist_1 = newRandomSound("music", "peace");
        // Fade out main theme and fade in session theme.
        crossFade(curr_music, curr_session_playlist_1, 0.1);
			// janwas: greatly accelerate this timesink;
			// will be replaced soon by native version that doesn't block.

        // Start refreshing the session controls.
	setInterval( snRefresh, 1, 100 );
}

// ====================================================================

function endSession (closeType)
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
                        crossFade(curr_session_playlist_1, curr_music, 0.1);
                                // janwas: greatly accelerate this timesink;
                                // will be replaced soon by native version that doesn't block.

			// Stop refreshing the session controls.
			cancelInterval();

                        // Swap GUIs to display main menu.
                        guiSwitch ("sn", "pg");
                break;
                case ("exit"):
                        // If the player has chosen to shutdown and immediately return to operating system,

                        exit();
                break;
        }
}

// ====================================================================
