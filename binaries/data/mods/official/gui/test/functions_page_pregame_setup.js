/*
	DESCRIPTION	: Functions for configuring a game prior to the loading screen (hosting, joining, or skirmish).
	NOTES		: 
*/

// ====================================================================

// Open the Session Setup screen.
function openSessionSetup (sessionReturnWindow)
{
	profileName 	= getCurrItemValue ("pgProfileName");
	titleBar	= getGUIObjectByName ("pgSessionSetupTitleBar");

	// Setup remaining slots.
	for (i = 1; i <= g_GameAttributes.numSlots; i++)
	{
		if (i == 1)
		{
			// Set the host player's name on the setup screen.
			pushItem ("pgSessionSetupP" + i, profileName + " (Host)");
		}
		else
		{
			pushItem ("pgSessionSetupP" + i, "Open");
			pushItem ("pgSessionSetupP" + i, "Closed");
			pushItem ("pgSessionSetupP" + i, "AI");
		}
		getGUIObjectByName ("pgSessionSetupP" + i).selected = 0;

		// Make objects non-interactive to client.
		if (sessionType == "Client") guiDisable ("pgSessionSetupP" + i);
	}

	// Set the title for the setup screen
	switch (sessionType)
	{
		case "Host":
			titleBar.caption = "Hosting: " + g_NetServer.serverName;
			// Reveal type-specific Session Setup objects.
			guiUnHide ("pgSessionSetupMP");
			guiUnHide ("pgSessionSetupMPHost");
			guiUnHide ("pgSessionSetupMaster");
		break;
		case "Client":
			titleBar.caption = "Joining: " // + g_NetClient.session[0].name;
			// Reveal type-specific Session Setup objects.
			guiUnHide ("pgSessionSetupMP");
			guiUnHide ("pgSessionSetupMPClient");

			// Disable objects that clients shouldn't be able to modify.
			guiDisable ("pgSessionSetupMapName");
			guiDisable ("pgSessionSetupStartButton");
		break;
		case "Skirmish":
			titleBar.caption = "Skirmish";
			// Reveal type-specific Session Setup objects.
			guiUnHide ("pgSessionSetupSkirmish");
			guiUnHide ("pgSessionSetupMaster");
		break;
		default:
		break;
	}

	// Reveal GUI.
	guiHide	("pgVersionNumber");
	if (sessionReturnWindow != "")
		guiSwitch (sessionReturnWindow, "pgSessionSetup");
	else
		openMainMenuSubWindow ("pgSessionSetup");
}

// ====================================================================

// Close/cancel the Session Setup screen.
function closeSessionSetup ()
{
	// Hide type-specific objects.
	guiHide ("pgSessionSetupMP");
	guiHide ("pgSessionSetupMPHost");
	guiHide ("pgSessionSetupMPClient");
	guiHide ("pgSessionSetupSkirmish");
	guiHide ("pgSessionSetupMaster");

	// Return to previous screen.
	switch (sessionType)
	{
		case "Host":
		case "Client":
			// terminate the multiplayer server session
			endGame();

			// Go back to multiplayer mode selection screen.
			guiSwitch ("pgSessionSetup", "pgMPModeSel");
		break;
		case "Skirmish":
			// Go back to main menu.
			closeMainMenuSubWindow ("pgSessionSetup");
		break;
		default:
		break;
	}

	guiUnHide ("pgVersionNumber");
}

// ====================================================================