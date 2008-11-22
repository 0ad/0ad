/*
	DESCRIPTION	: Functions for the hosting and joining multiplayer games.
	NOTES		: 
*/

// ====================================================================

function initMPHost (parentWindow, gameName, welcomeMessage, profileName)
{
	// Set up the server
	var server = createServer();
	// Welcome message
	server.welcomeMessage = welcomeMessage;
	// Server Name
	server.serverName = gameName;
	// Host Name
	server.serverPlayerName = profileName;

	// start listening
	var success = server.open();
	if (!success)
	{
		messageBox(400, 200, "Failed to start server. Please review the logfile for more information on the problem.", "Problem", 2, new Array(), new Array());
		return 1;
	}
	else
	{
		console.write ("Host has started listening for client connections.");

		// Switch to Session Setup screen.
		sessionType = "Host";
		openSessionSetup (parentWindow);
	}

	// -------------------------
	// INCOMING CONNECTIONS
	// -------------------------
	server.onClientConnect = function (event) 
	{
		console.write ("Client connected.");
		console.write("A new client has successfully connected! ID: " + event.id + ", Name: " + event.name + ", Session: " + event.session);

		// Assign newly connected client to next available slot.	
		var playerSlot = g_GameAttributes.getOpenSlot();
		playerSlot.assignToSession (event.session);
		// Set player slot to new player's name
		
		var slotID = g_GameAttributes.getUsedSlotsAmount;
		pushItem ("pgSessionSetupP" + slotID, g_GameAttributes.slots[slotID-1].player.name);
	}
	
	server.onClientDisconnect = function (event) 
	{
		// Client has disconnected; free their slot.
		var slotID = g_GameAttributes.getUsedSlotsAmount-1;
		g_GameAttributes.slots[slotID].assignOpen();
		var result = setCurrItemValue ("pgSessionSetupP" + slotID, "Open");

		messageBox(400, 200, "Client " + event.name + "(" + event.id + ") disconnected from " + event.session + ".", "Client Disconnected", 2, new Array(), new Array());
	}	

	server.onChat = function (event) 
	{
		messageBox(400, 200, "(" + event.sender + ")" + event.message, "Host Chat Message", 2, new Array(), new Array());
	}

	// Need "waiting for more players to join and start game" code here
}

// ====================================================================

function initMPClient (mpParentWindow, ipAddress, profileName)
{
	// Create the client instance
	var client = createClient();
	// Player Name
	client.playerName = profileName;

	client.onClientConnect = function (event)
	{
		// Set player slot to new player's name.
		console.write("onClientConnect: name is " + event.name + ", event id is " + event.id);
		
		var slotID = g_GameAttributes.getUsedSlotsAmount;
		console.write("slotID="+slotID);
		pushItem ("pgSessionSetupP" + eval(slotID+1), g_GameAttributes.slots[slotID].player.name);
	}

	client.onConnectComplete = function (event)
	{
		console.write("onConnectComplete: " + event.message);
		if (event.success)
		{
			// Switch to Session Setup screen.
			sessionType = "Client";
			openSessionSetup ("pgMPJoin");

			messageBox (400, 200, "Client connected " + event.message + ".",
				"Connect Complete", 2, new Array(), new Array());
		}
		else
		{
			messageBox (400, 200, "Client failed to complete connection: " + event.message + ".",
				"Connect Failure", 2, new Array(), new Array());
		}
	}

	client.onChat = function (event)
	{
		messageBox (400, 200, "(" + event.sender + ")" + event.message,
			"Client Chat Message", 2, new Array(), new Array());
	}

	client.onDisconnect = function (event)
	{
		console.write("onDisconnect: " + event.message);
		messageBox (400, 200, event.message,
			"Host Disconnected", 2, new Array(), new Array());
	}

	client.onClientDisconnect = function (event)
	{
		console.write("onClientDisconnect: " + event.message);
		messageBox (400, 200, event.session,
			"Client " + event.name + "(" + event.id + ") Disconnected", 2, new Array(), new Array());
	}

	client.onStartGame = function (event)
	{
		// The server has called Start Game!, so we better switch to the session GUI and stuff like that.
		launchGame();
	}
	
	var success = client.beginConnect (ipAddress);
	if (!success)
	{
		messageBox(400, 200, "Failed to connect to server. Please check the network connection.",
			"Client Failure", 2, new Array(), new Array());
		return 1;
	}
	else
	{
		console.write ("Client successfully started to connect to " + ipAddress + ".");
	}
}

// ====================================================================
