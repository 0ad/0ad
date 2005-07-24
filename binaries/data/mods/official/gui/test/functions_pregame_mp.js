// ====================================================================

function initMPSessionHost()
{
	var gameName = getGUIObjectByName("pregame_mp_host_gamename").caption;
	guiSwitch ("pregame_mp_host", "pregame_mp_setup_host");
	guiHide("pregame_mainmenu_versionnumber");
 
	// Set up the server
	var server = createServer();
	// Welcome message
	server.welcomeMessage = getGUIObjectByName("pregame_mp_host_welcomemsg").caption;
	// Server Name
	server.serverName = gameName;
                              	
	// start listening
	var success = server.open();
	if(!success) {
		messageBox(400, 200, "Failed to start server. Please review the logfile for more information on the problem.", "Problem", 2, new Array(), new Array());
	}

	// Set the title for the setup screen
	getGUIObjectByName("pregame_mp_setup_host_titlebar").caption = "Hosting: " + getGUIObjectByName("pregame_mp_host_gamename").caption;

	// Set the host player's name on the setup screen
	getGUIObjectByName("pregame_mp_setup_host_p1_txt").caption = "P1: " + getGUIObjectByName("pregame_mp_modesel_playername").caption + " (Host)";
        
	// Set the map to use
	//g_GameAttributes.mapFile = mapName;

	// -------------------------
	// INCOMING CONNECTIONS
	// -------------------------
	server.onClientConnect=function (event) {
		console.write("Client connected.");
		//console.write("A new client has successfully connected! ID: " + event.id + ", Name: " + event.name + ", Session: " + event.session);
		//console.write("New client.");
		//var playerSlot = g_GameAttributes.getOpenSlot();
		// assign a slot
		//playerSlot.assignToSession(event.session);
		//console.write("slot: " + playerSlot.player);
		//console.write("got here");
		// need to refresh the dialog control data here
	};
	
	server.onClientDisconnect=function (event) {
		console.write("Client disconnected.");
	};	
        
	// CHAT
	/*server.onChat=function (event) {
		messageBox(400, 200, event.sender+" says: "+event.message, "Chat Message", 2, new Array(), new Array());
	};*/

	// Need "waiting for more players to join and start game" code here

	/*btCaptions = new Array("OK");
	btCode = new Array("startLoadingScreen();");
	messageBox(400, 200, "Waiting for clients to join - Click OK to start the game.", "Ready", 0, btCaptions, btCode);*/
	
}

// ====================================================================

function initMPSessionClient(playerName, serverIP)
{
        GUIObjectHide("pregame_mp_ip");
        GUIObjectHide("pregame_subwindow_bkg");

        var client=createClient();
        
        client.playerName=playerName;
        
        client.onStartGame=function () {
                messageBox(400, 200, "The game starts now!!!", "Get Ready!", 2, new Array(), new Array());
                startLoadingScreen();
        };
        
        client.onChat=function (event) {
                messageBox(400, 200, event.sender+" says: "+event.message, "Chat Message", 2, new Array(), new Array());
        };
        
        client.onConnectComplete=function (event) {
                messageBox(400, 200, "Result message: "+event.message, "Connect complete", 2, new Array(), new Array());
        };
        
        // Join MP game
        var success = client.beginConnect(serverIP);
        if(!success) {
                messageBox(400, 200, "Failed to join game. Please review the logfile for more information on the problem.", "Failure", 2, new Array(), new Array());
        }

        // Need "waiting for game to start" code here - it should automatically start if the client recieves the start signal from the server,
        // but I currently don't know how that could be done.

        /*btCaptions = new Array("OK");
        btCode = new Array("");
        messageBox(400, 200, "Data: " + playerName + ";" + serverIP + ";", "Test", 0, btCaptions, btCode);*/
}

// ====================================================================

