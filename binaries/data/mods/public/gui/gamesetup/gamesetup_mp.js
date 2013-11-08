var g_IsConnecting = false;
var g_GameType; // "server" or "client"
var g_ServerName = "";

var g_IsRejoining = false;
var g_GameAttributes; // used when rejoining
var g_PlayerAssignments; // used when rejoining

function init(attribs)
{
	switch (attribs.multiplayerGameType)
	{
	case "join":
		if(Engine.HasXmppClient())
		{
			if (startJoin(attribs.name, attribs.ip))
				switchSetupPage("pageJoin", "pageConnecting");
		}
		else
		{
			getGUIObjectByName("pageJoin").hidden = false;
			getGUIObjectByName("pageHost").hidden = true;
		}
		break;
	case "host":
		getGUIObjectByName("pageJoin").hidden = true;
		getGUIObjectByName("pageHost").hidden = false;
		if(Engine.HasXmppClient())
		{
			getGUIObjectByName("hostServerNameWrapper").hidden = false;
			getGUIObjectByName("hostPlayerName").caption = attribs.name;
			getGUIObjectByName("hostServerName").caption = attribs.name + "'s game";
		}
		else
			getGUIObjectByName("hostPlayerNameWrapper").hidden = false;
		break;
	default:
		error("Unrecognised multiplayer game type : " + attribs.multiplayerGameType);
		break;
	}
}

function cancelSetup()
{
	if (g_IsConnecting)
		Engine.DisconnectNetworkGame();
	// Set player lobby presence
	if (Engine.HasXmppClient())
		Engine.LobbySetPlayerPresence("available");
	Engine.PopGuiPage();
}

function startConnectionStatus(type)
{
	g_GameType = type;
	g_IsConnecting = true;
	g_IsRejoining = false;
	getGUIObjectByName("connectionStatus").caption = "Connecting to server...";
}

function onTick()
{
	if (!g_IsConnecting)
		return;

	pollAndHandleNetworkClient();
}

function pollAndHandleNetworkClient()
{
	while (true)
	{
		var message = Engine.PollNetworkClient();
		if (!message)
			break;

		log("Net message: "+uneval(message));

		// If we're rejoining an active game, we don't want to actually display
		// the game setup screen, so perform similar processing to gamesetup.js
		// in this screen
		if (g_IsRejoining)
		{
			switch (message.type)
			{
			case "netstatus":
				switch (message.status)
				{
				case "disconnected":
					cancelSetup();
					reportDisconnect(message.reason);
					return;

				default:
					error("Unrecognised netstatus type "+message.status);
					break;
				}
				break;

			case "gamesetup":
				g_GameAttributes = message.data;
				break;

			case "players":
				g_PlayerAssignments = message.hosts;
				break;

			case "start":
				Engine.SwitchGuiPage("page_loading.xml", {
					"attribs": g_GameAttributes,
					"isNetworked" : true,
					"playerAssignments": g_PlayerAssignments
				});
				break;

			case "chat":
				// Ignore, since we have nowhere to display chat messages
				break;

			default:
				error("Unrecognised net message type "+message.type);
			}
		}
		else
		{
			// Not rejoining - just trying to connect to server

			switch (message.type)
			{
			case "netstatus":
				switch (message.status)
				{
				case "connected":
					getGUIObjectByName("connectionStatus").caption = "Registering with server...";
					break;

				case "authenticated":
					if (message.rejoining)
					{
						getGUIObjectByName("connectionStatus").caption = "Game has already started - rejoining...";
						g_IsRejoining = true;
						return; // we'll process the game setup messages in the next tick
					}
					else
					{
						Engine.SwitchGuiPage("page_gamesetup.xml", { "type": g_GameType, "serverName": g_ServerName });
						return; // don't process any more messages - leave them for the game GUI loop
					}

				case "disconnected":
					cancelSetup();
					reportDisconnect(message.reason);
					return;

				default:
					error("Unrecognised netstatus type "+message.status);
					break;
				}
				break;
			default:
				error("Unrecognised net message type "+message.type);
				break;
			}
		}
	}
}

function switchSetupPage(oldpage, newpage)
{
	getGUIObjectByName(oldpage).hidden = true;
	getGUIObjectByName(newpage).hidden = false;
}

function startHost(playername, servername)
{
	// Disallow identically named games in the multiplayer lobby
	if (Engine.HasXmppClient())
	{
		for each (g in Engine.GetGameList())
		{
			if (g.name === servername)
			{
				getGUIObjectByName("hostFeedback").caption = "Game name already in use.";
				return false;
			}
		}
	}
	try
	{
		Engine.StartNetworkHost(playername);
	}
	catch (e)
	{
		cancelSetup();
		messageBox(400, 200,
			"Cannot host game: " + e.message + ".",
			"Error", 2);
		return false;
	}

	startConnectionStatus("server");
	g_ServerName = servername;
	// Set player lobby presence
	if (Engine.HasXmppClient())
		Engine.LobbySetPlayerPresence("playing");
	return true;
}

function startJoin(playername, ip)
{
	try
	{
		Engine.StartNetworkJoin(playername, ip);
	}
	catch (e)
	{
		cancelSetup();
		messageBox(400, 200,
			"Cannot join game: " + e.message + ".",
			"Error", 2);
		return false;
	}

	startConnectionStatus("client");
	// Set player lobby presence
	if (Engine.HasXmppClient())
		Engine.LobbySetPlayerPresence("playing");
	return true;
}
