var g_IsConnecting = false;
var g_GameType; // "server" or "client"
var g_ServerName = "";

var g_IsRejoining = false;
var g_GameAttributes; // used when rejoining
var g_PlayerAssignments; // used when rejoining
var g_userRating; // player rating

function init(attribs)
{
	g_userRating = attribs.rating;
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
			Engine.GetGUIObjectByName("pageJoin").hidden = false;
			Engine.GetGUIObjectByName("pageHost").hidden = true;
		}
		break;
	case "host":
		Engine.GetGUIObjectByName("pageJoin").hidden = true;
		Engine.GetGUIObjectByName("pageHost").hidden = false;
		if(Engine.HasXmppClient())
		{
			Engine.GetGUIObjectByName("hostServerNameWrapper").hidden = false;
			Engine.GetGUIObjectByName("hostPlayerName").caption = attribs.name;
			Engine.GetGUIObjectByName("hostServerName").caption = sprintf(translate("%(name)s's game"), { name: attribs.name });
		}
		else
			Engine.GetGUIObjectByName("hostPlayerNameWrapper").hidden = false;
		break;
	default:
		error(sprintf("Unrecognised multiplayer game type: %(gameType)s", { gameType: multiplayerGameType }));
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
	Engine.GetGUIObjectByName("connectionStatus").caption = translate("Connecting to server...");
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

		log(sprintf(translate("Net message: %(message)s"), { message: uneval(message) }));

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
					error(sprintf("Unrecognised netstatus type %(netType)s", { netType: message.status }));
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
				error(sprintf("Unrecognised net message type %(messageType)s", { messageType: message.type }));
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
					Engine.GetGUIObjectByName("connectionStatus").caption = translate("Registering with server...");
					break;

				case "authenticated":
					if (message.rejoining)
					{
						Engine.GetGUIObjectByName("connectionStatus").caption = translate("Game has already started, rejoining...");
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
					error(sprintf("Unrecognised netstatus type %(netType)s", { netType: message.status }));
					break;
				}
				break;
			default:
				error(sprintf("Unrecognised net message type %(messageType)s", { messageType: message.type }));
				break;
			}
		}
	}
}

function switchSetupPage(oldpage, newpage)
{
	Engine.GetGUIObjectByName(oldpage).hidden = true;
	Engine.GetGUIObjectByName(newpage).hidden = false;
}

function startHost(playername, servername)
{
	// Disallow identically named games in the multiplayer lobby
	if (Engine.HasXmppClient())
	{
		for each (var g in Engine.GetGameList())
		{
			if (g.name === servername)
			{
				Engine.GetGUIObjectByName("hostFeedback").caption = translate("Game name already in use.");
				return false;
			}
		}
	}
	try
	{
		if (g_userRating)
			Engine.StartNetworkHost(playername + " (" + g_userRating + ")");
		else
			Engine.StartNetworkHost(playername);
	}
	catch (e)
	{
		cancelSetup();
		messageBox(400, 200,
			sprintf("Cannot host game: %(message)s.", { message: e.message }),
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
		if (g_userRating)
			Engine.StartNetworkJoin(playername + " (" + g_userRating + ")", ip);
		else
			Engine.StartNetworkJoin(playername, ip);
	}
	catch (e)
	{
		cancelSetup();
		messageBox(400, 200,
			sprintf("Cannot join game: %(message)s.", { message: e.message }),
			"Error", 2);
		return false;
	}

	startConnectionStatus("client");
	// Set player lobby presence
	if (Engine.HasXmppClient())
		Engine.LobbySetPlayerPresence("playing");
	return true;
}

function getDefaultGameName()
{
	return sprintf(translate("%(playername)s's game"), { playername: Engine.ConfigDB_GetValue("user", "playername")});
}
