/*
	DESCRIPTION	: Functions used load/end the game session and manipulate the loading screen.
	NOTES	: 
*/

// ====================================================================

function launchGame ()
{
	// Starts the map, closing the session setup window.
	
	var mapName = getCurrItemValue("pgSessionSetupMapName");
	var gameMode = getCurrItemValue("pgSessionSetupGameMode");
	var screenshotMode = getGUIObjectByName("pgSessionSetupScreenshotMode").checked;
	var losSetting = getGUIObjectByName("pgSessionSetupLosSetting").selected;
	var fogOfWar = getGUIObjectByName("pgSessionSetupFoW").checked;
	if(screenshotMode)
	{
		losSetting = 2;
		fogOfWar = false;
	}
	
	// Check whether we have a correct file extension, to avoid crashes
	var extension = mapName.substring (mapName.length, mapName.length-4);
	if (extension != ".pmp")
	{
		// Add .pmp to the file name.
		var mapName =  mapName + ".pmp";
	}

	// Set up game
	g_GameAttributes.mapFile = mapName;
	g_GameAttributes.losSetting = losSetting;
	g_GameAttributes.fogOfWar = fogOfWar;
	g_GameAttributes.gameMode = gameMode
	g_GameAttributes.screenshotMode = screenshotMode;
	
	// Close setup window
	closeMainMenuSubWindow ("pgSessionSetup");

	// Display loading screen.	
	startLoadingScreen();

	console.write( "running startGame()" );
	
	// Begin game session.
	if (! startGame())
	{
		// Failed to start the game; go back to the main menu.
		guiSwitch ("ld", "pg");
		// Restore default cursor.
		setCursor ("arrow-default");
		// Show an error message
		btCaptions = new Array("OK");
		btCode = new Array("");
		messageBox(400, 200, "The game could not be started with the given parameters. You probably have entered an invalid map name.", "Error", 0, btCaptions, btCode);
	}
	
	console.write( "done running startGame()" );

	// Set starting UI layout.
	GUIType=rb;

	// Set session UI sprites to match the skin for the player's civilisation.
	// (We don't have skins for all civs yet, so we're using the standard menu skin. But it should be settable from here later.)
	setSkin ("wheat");	
	
	// Set GUI coordinates to starting orientation.
	flipGUI (GUIType);	
}

// ====================================================================

function startLoadingScreen()
{
	// Switch screens from main menu to loading screen.
	guiSwitch ("pg", "ld");
	// Set to "hourglass" cursor.
	setCursor("cursor-wait");
	console.write ("Loading " + g_GameAttributes.mapFile + " (" + g_GameAttributes.numPlayers + " players) ...");

	// Choose random concept art for loading screen background (should depend on the selected civ later when this is specified).
	var sprite = "";
	var loadingBkgArray = buildDirEntList("art/textures/ui/loading/", "*.dds", false);
	if (loadingBkgArray.length == 0)
		console.write ("ERROR: Failed to find any matching textures for the loading screen background.");
	else
	{
		// Get a random index from the list of loading screen backgrounds.
		sprite = "stretched:" + loadingBkgArray[getRandom (0, loadingBkgArray.length-1)];
		sprite = sprite.replace ("art/textures/ui/", "");
	}
	getGUIObjectByName ("ldConcept").sprite = sprite;

	// janwas: main loop now sets progress / description, but that won't
	// happen until the first timeslice completes, so set initial values.
	getGUIObjectByName ("ldTitleBar").caption = "Loading Scenario ...";
	getGUIObjectByName ("ldProgressBarText").caption = "";
	getGUIObjectByName ("ldProgressBar").caption = 0;
	getGUIObjectByName ("ldText").caption = "LOADING " + g_GameAttributes.mapFile + " ...\nPlease wait ...";

	// Pick a random tip of the day (each line is a separate tip).
	var tipArray  = readFileLines("gui/text/tips.txt");
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
	
	// Restore default cursor.
	setCursor ("arrow-default");
}

// ====================================================================

function setupSession ()
{
	// Do essentials that can only be done when the session has been loaded ...
	// For example, create the resource types, scores, etc, for each player.

/*
	if (sessionType == "Skirmish")
	{
	// Set up a bunch of players so we can see them pretty colours. :P
	console.write ("Setting Up Temporary Single Players");
	// The first player is by default allocated to the local player in SP, so
	// adding 7 players means that we'll have 8 players in total
	for (var i=0; i<7; i++)
	{
		g_GameAttributes.slots[i+1].assignLocal();
		console.write ("Slot "+(i+1)+" is assigned: " + g_GameAttributes.slots[i+1].assignment);
	}
	}
*/

	// Select session peace track.
	curr_session_playlist_1 = newRandomSound("music", "peace");
	// Fade out main theme and fade in session theme.
	crossFade(curr_music, curr_session_playlist_1, 1)

	// Create the resouce counters
	createResourceCounters();
	
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
		//curr_music = newRandomSound("music", "menu");
		//crossFade(curr_session_playlist_1, curr_music, 1);

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
