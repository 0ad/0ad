function initIPHost()
{
        // IP Host Window background.
        crd_pregame_iphost_bkg_x = -250;
        crd_pregame_iphost_bkg_y = -200;
        crd_pregame_iphost_bkg_width = (crd_pregame_iphost_bkg_x * -1) * 2;
        crd_pregame_iphost_bkg_height = (crd_pregame_iphost_bkg_y * -1) * 2;

        // IP Host Window exit button.
        crd_pregame_iphost_exit_button_width = 16;
        crd_pregame_iphost_exit_button_height = crd_pregame_iphost_exit_button_width;
        crd_pregame_iphost_exit_button_x = crd_pregame_iphost_bkg_x+crd_pregame_iphost_bkg_width+10;
        crd_pregame_iphost_exit_button_y = crd_pregame_iphost_bkg_y-25;

        // IP Host Window titlebar.
        crd_pregame_iphost_titlebar_width = crd_pregame_iphost_bkg_width;
        crd_pregame_iphost_titlebar_height = 16;
        crd_pregame_iphost_titlebar_x = crd_pregame_iphost_bkg_x;
        crd_pregame_iphost_titlebar_y = crd_pregame_iphost_bkg_y-25;
}

// ====================================================================

function initMPSessionHost(playerName, mapName)
{
	var server = createServer();
	
	// Set basic server options, such as:
	// server.port = 20595; // Default is 20595 - you can also explicitly set to -1 for default port
	server.serverPlayerName=playerName;
	server.serverName=playerName+"'s Server";
	server.welcomeMessage="Welcome to "+server.serverName;

	// Actually start listening for connections.. This should probably not be
	// done until there's been a dialog for filling in the previous options ;-)
	var success = server.open();
	if(!success) {
        	messageBox(400, 200, "Failed to start server. Please review the logfile for more information on the problem.", "Problem", 2, new Array(), new Array());
	}

	GUIObjectHide("pregame_mp_ip");
	GUIObjectHide("pregame_subwindow_bkg");

	// Need "waiting for more players to join and start game" code here

	/*btCaptions = new Array("OK");
	btCode = new Array("");
	messageBox(400, 200, "Data: " + playerName + ";" + mapName + ";", "Test", 0, btCaptions, btCode);*/
}

// ====================================================================

function initMPSessionClient(playerName, serverIP)
{
	var client=createClient();
	
	client.playerName=playerName;
	
	client.onStartGame=function () {
		messageBox(400, 200, "The game starts now!!!", "Get Ready!", 2, new Array(), new Array());
		startLoadingScreen();
	};
	
	client.onChat=function (event) {
		messageBox(400, 200, event.message, "Chat Message", 2, new Array(), new Array());
	};
	
	client.onConnectComplete=function (event) {
		messageBox(400, 200, "Result message: "+event.message, "Connect complete", 2, new Array(), new Array());
	};
	
	// Join MP game
	var success = client.beginConnect(serverIP);
	if(!success) {
        	messageBox(400, 200, "Failed to join game. Please review the logfile for more information on the problem.", "Failure", 2, new Array(), new Array());
	}

	GUIObjectHide("pregame_mp_ip");
	GUIObjectHide("pregame_subwindow_bkg");

	// Need "waiting for game to start" code here - it should automatically start if the client recieves the start signal from the server,
	// but I currently don't know how that could be done.

	/*btCaptions = new Array("OK");
	btCode = new Array("");
	messageBox(400, 200, "Data: " + playerName + ";" + serverIP + ";", "Test", 0, btCaptions, btCode);*/
}

// ====================================================================

