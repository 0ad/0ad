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
        
        GUIObjectHide("loading_screen");
        GUIObjectUnhide("session_gui");
        FlipGUI(GUIType);

        // Select session peace track.
        curr_session_playlist_1 = newRandomSound("music", "peace");
        // Fade out main theme and fade in session theme.
        CrossFade(curr_music, curr_session_playlist_1, 0.0001);
}

// ====================================================================
