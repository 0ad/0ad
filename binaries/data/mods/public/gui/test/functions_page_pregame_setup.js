/*
	DESCRIPTION	: Functions for configuring a game prior to the loading screen (hosting, joining, or skirmish).
	NOTES		: 
*/

// ====================================================================

// Open the Session Setup screen.
function openSessionSetup (sessionReturnWindow)
{
	var profileName 	= getCurrItemValue ("pgProfileName");
	var titleBar		= getGUIObjectByName ("pgSessionSetupTitleBar");

	// Setup remaining slots.
	for (var i = 1; i <= g_GameAttributes.numSlots; i++)
	{
		if (i == 1)
		{
			// Set the host player's name on the setup screen.
			getGUIObjectByName ("pgSessionSetupP" + i).caption = "(Host)";
		}
		else
		{
//			pushItem ("pgSessionSetupP" + i, "session");
			pushItem ("pgSessionSetupP" + i, "Open");
			pushItem ("pgSessionSetupP" + i, "Closed");
//			pushItem ("pgSessionSetupP" + i, "local");
//			pushItem ("pgSessionSetupP" + i, "AI");

			// Set other slots to Open.
			var result = setCurrItemValue ("pgSessionSetupP" + i, "Open");

			// Setup onchange event (closing and opening affects host slots).
			getGUIObjectByName ("pgSessionSetupP" + i).onSelectionChange = function (event)
			{
				var slotNumber = this.name.substring (this.name.length-1, this.name.length);
				switch (getCurrItemValue (this.name))
				{
					case "Open":
						// If "Open" selected, 				

						// Open the slot.
						g_GameAttributes.slots[slotNumber-1].assignOpen();

						//console.write ("Opened slot " + this.name);
					break;
					case "Closed":
						// If "Closed" selected,

						// And slot is occupied,
						if (g_GameAttributes.slots[slotNumber-1].assignment == "session")
						{
							// Remove player name from slot list.
							removeItem (this.name, g_GameAttributes.slots[slotNumber-1].player);

							// Prompt that player was kicked.
							// (Change this to a chat message when functionality available.)
							console.write (g_GameAttributes.slots[slotNumber-1].player + " was kicked by the host.");
						}	
	
						// Close the slot.
						g_GameAttributes.slots[slotNumber-1].assignClosed();

						//console.write ("Closed slot " + this.name);
					break;
					default:
					break;
				}
			}


		}

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
			titleBar.caption = "Joining: " // g_NetServer.serverName;
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

