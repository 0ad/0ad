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
        setTimeout( loadSession, 200 );
}

// ====================================================================

function loadSession()
{
        if (! startGame())
        {
                // Failed to start the game; go back to the main menu. TODO: display an error message.
                GUIObjectHide("loading_screen");
                GUIObjectUnhide("pregame_gui");
                return;
        }

	// Create resource pools for each player, etc.
	setupSession();        

	// Switch GUI from main menu to game session.
        GUIObjectHide("loading_screen");
        GUIObjectUnhide("session_gui");
        FlipGUI(GUIType);

        // Select session peace track.
        curr_session_playlist_1 = newRandomSound("music", "peace");
        // Fade out main theme and fade in session theme.
        CrossFade(curr_music, curr_session_playlist_1, 0.0001);
}

// ====================================================================

function setupSession()
{
	// Do essentials that can only be done when the session has been loaded ...
	// For example, create the resource types.
	// Initialise Resource Pools by attaching them to the Player object.
	// (CPlayer code takes care of giving a copy to each player.)
	player = new Object(); // I shouldn't need to do this. Need to find the existing Player to add these to.
	player.resource = new Object();
	player.resource.food = 0;
	player.resource.wood = 0;
	player.resource.stone = 0;
	player.resource.ore = 0;
	player.resource.pop = new Object();
	player.resource.pop.curr = 0;
	player.resource.pop.housing = 0;

	// Start refreshing the session controls.
	setInterval( getObjectInfo, 1, 1000 );
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
			curr_music = newRandomSound('music', 'theme');
			CrossFade(curr_session_playlist_1, curr_music, 0.0001);

			// Swap GUIs to display main menu.
			GUIObjectHide('session_gui');
			GUIObjectUnhide('pregame_gui');
		break;
		case ("exit"):
			// If the player has chosen to shutdown and immediately return to operating system,

			exit();
		break;
	}
}

// ====================================================================