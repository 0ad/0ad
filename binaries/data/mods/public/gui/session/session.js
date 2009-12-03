function init()
{
	// Set starting UI layout.
	GUIType=rb;

	// Set session UI sprites to match the skin for the player's civilisation.
	// (We don't have skins for all civs yet, so we're using the standard menu skin. But it should be settable from here later.)
	setSkin ("wheat");	
	
	// Set GUI coordinates to starting orientation.
	flipGUI (GUIType);	

	// Create resource pools for each player, etc.
	setupSession();  
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
		switchGuiPage("page_pregame.xml");
		break;
	case ("exit"):
		// If the player has chosen to shutdown and immediately return to operating system,
		exit();
		break;
	}
}

