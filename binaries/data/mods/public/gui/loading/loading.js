function init()
{
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
	// Stop the music
	if (curr_music)
		curr_music.fade(-1, 0.0, 5.0); // fade to 0 over 5 seconds

	// This is a reserved function name that is executed by the engine when it is ready
	// to start the game (ie loading progress has reached 100%).

	// Switch GUI from loading screen to game session.
	if (Engine.IsNewSimulation())
		Engine.SwitchGuiPage("page_session_new.xml");
	else
		Engine.SwitchGuiPage("page_session.xml");
	
	// Restore default cursor.
	setCursor ("arrow-default");
}

