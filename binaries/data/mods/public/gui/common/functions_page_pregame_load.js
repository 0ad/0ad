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
	Engine.SwitchGuiPage("page_loading.xml");

	console.write( "running startGame()" );
	
	// Begin game session.
	if (! startGame())
	{
		// Failed to start the game; go back to the main menu.
		Engine.SwitchGuiPage("page_pregame.xml");
		// Restore default cursor.
		setCursor ("arrow-default");
		// Show an error message
		messageBox(400, 200, "The game could not be started with the given parameters. You probably have entered an invalid map name.", "Error", 0, ["OK"], [undefined]);
	}
	
	console.write( "done running startGame()" );
}

// ====================================================================
