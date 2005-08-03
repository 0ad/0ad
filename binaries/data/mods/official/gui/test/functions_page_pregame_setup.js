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
	slotP1		= getGUIObjectByName ("pgSessionSetupP1Txt");

	// Set the host player's name on the setup screen.
	slotP1.caption 	= "P1: " + profileName + " (Host)";

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
			titleBar.caption = "Joining: " + g_NetServer.serverName;
			// Reveal type-specific Session Setup objects.
			guiUnHide ("pgSessionSetupMP");
			guiUnHide ("pgSessionSetupMPClient");
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