// ====================================================================

function initMPSessionHost(playerName, mapName)
{
        GUIObjectHide("pregame_mp_ip");
        GUIObjectHide("pregame_subwindow_bkg");

        var server = createServer();
        
        // Set the map to use
        g_GameAttributes.mapFile = mapName;
        
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

        server.onChat=function (event) {
                messageBox(400, 200, event.sender+" says: "+event.message, "Chat Message", 2, new Array(), new Array());
        };

        // Need "waiting for more players to join and start game" code here

        btCaptions = new Array("OK");
        btCode = new Array("startLoadingScreen();");
        messageBox(400, 200, "Waiting for clients to join - Click OK to start the game.", "Ready", 0, btCaptions, btCode);
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

