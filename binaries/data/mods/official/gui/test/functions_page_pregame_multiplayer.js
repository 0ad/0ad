/*
	DESCRIPTION	: Functions for the hosting and joining multiplayer games.
	NOTES		: 
*/

// ====================================================================

function initMPHost (parentWindow, gameName, welcomeMessage, profileName)
{
	// Set up the server
	server = createServer();
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
	}

	// -------------------------
	// INCOMING CONNECTIONS
	// -------------------------
	server.onClientConnect = function (event) 
	{
		console.write("Client connected.");
		//console.write("A new client has successfully connected! ID: " + event.id + ", Name: " + event.name + ", Session: " + event.session);
		//console.write("New client.");
		var playerSlot = g_GameAttributes.getOpenSlot();
		// assign a slot
		playerSlot.assignToSession(event.session);
		//console.write("slot: " + playerSlot.player);
		//console.write("got here");
		// need to refresh the dialog control data here
	}
	
	server.onClientDisconnect = function (event) 
	{
		console.write("Client disconnected.");
	}	
        
	// CHAT
	server.onChat = function (event) 
	{
		messageBox(400, 200, event.sender+" says: "+event.message, "Chat Message", 2, new Array(), new Array());
	}

	// Need "waiting for more players to join and start game" code here

	// Switch to Session Setup screen.
	sessionType = "Host";
	openSessionSetup (parentWindow);
}

// ====================================================================

function initMPClient (parentWindow, ipAddress, profileName)
{
	// Create the client instance
	client = createClient();
	// Player Name
	client.playerName = profileName;
	
	success = client.beginConnect (ipAddress);
	if (!success)
	{
		// need proper message box code here later
		console.write ("Failed to connect to server. Please check the network connection.");
	}
	else
	{
		// see above
		console.write ("Client successfully started to connect.");
	}
				
	client.onConnectComplete = function (event)
	{
		messageBox (400, 200, "Result message: "+event.message, "Connect complete", 2, new Array(), new Array());

		// Switch to Session Setup screen.
		sessionType = "Client";
		openSessionSetup (parentWindow);

	}
}

// ====================================================================