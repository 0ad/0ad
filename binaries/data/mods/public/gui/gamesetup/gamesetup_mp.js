var g_IsConnecting = false;
var g_GameType; // "server" or "client"
var g_ServerName = "";

var g_IsRejoining = false;
var g_GameAttributes; // used when rejoining
var g_PlayerAssignments; // used when rejoining
var g_UserRating;

function init(attribs)
{
	g_UserRating = attribs.rating;
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
			Engine.GetGUIObjectByName("hostServerName").caption =
				sprintf(translate("%(name)s's game"), { "name": attribs.name });
		}
		else
			Engine.GetGUIObjectByName("hostPlayerNameWrapper").hidden = false;
		break;
	default:
		error("Unrecognised multiplayer game type: " + attribs.multiplayerGameType);
		break;
	}
}

function cancelSetup()
{
	if (g_IsConnecting)
		Engine.DisconnectNetworkGame();

	if (Engine.HasXmppClient())
		Engine.LobbySetPlayerPresence("available");

	Engine.PopGuiPage();
}

function confirmSetup()
{
	if (!Engine.GetGUIObjectByName("pageJoin").hidden)
	{
		let joinPlayerName = Engine.GetGUIObjectByName("joinPlayerName").caption;
		let joinServer = Engine.GetGUIObjectByName("joinServer").caption;
		if (startJoin(joinPlayerName, joinServer))
			switchSetupPage("pageJoin", "pageConnecting");
	}
	else if (!Engine.GetGUIObjectByName("pageHost").hidden)
	{
		let hostPlayerName = Engine.GetGUIObjectByName("hostPlayerName").caption;
		let hostServerName = Engine.GetGUIObjectByName("hostServerName").caption;
		if (startHost(hostPlayerName, hostServerName))
			switchSetupPage("pageHost", "pageConnecting");
	}
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

		log(sprintf(translate("Net message: %(message)s"), { "message": uneval(message) }));

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
					reportDisconnect(message.reason, false);
					return;

				default:
					error("Unrecognised netstatus type: " + message.status);
					break;
				}
				break;

			case "gamesetup":
				g_GameAttributes = message.data;
				break;

			case "players":
				g_PlayerAssignments = message.newAssignments;
				break;

			case "start":

				// Copy playernames from initial player assignment to the settings
				for (let guid in g_PlayerAssignments)
				{
					let player = g_PlayerAssignments[guid];
					if (player.player > 0)	// not observer or GAIA
						g_GameAttributes.settings.PlayerData[player.player - 1].Name = player.name;
				}

				Engine.SwitchGuiPage("page_loading.xml", {
					"attribs": g_GameAttributes,
					"isNetworked" : true,
					"isRejoining" : g_IsRejoining,
					"playerAssignments": g_PlayerAssignments
				});
				break;

			case "chat":
				break;

			case "netwarn":
				break;

			default:
				error("Unrecognised net message type: " + message.type);
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
					reportDisconnect(message.reason, false);
					return;

				default:
					error("Unrecognised netstatus type: " + message.status);
					break;
				}
				break;

			case "netwarn":
				break;

			default:
				error("Unrecognised net message type: " + message.type);
				break;
			}
		}
	}
}

function switchSetupPage(oldpage, newpage)
{
	Engine.GetGUIObjectByName(oldpage).hidden = true;
	Engine.GetGUIObjectByName(newpage).hidden = false;
	Engine.GetGUIObjectByName("continueButton").hidden = true;
}

function startHost(playername, servername)
{
	// Save player name
	Engine.ConfigDB_CreateValue("user", "playername.multiplayer", playername);
	Engine.ConfigDB_WriteValueToFile("user", "playername.multiplayer", playername, "config/user.cfg");

	// Disallow identically named games in the multiplayer lobby
	if (Engine.HasXmppClient())
	{
		for (let g of Engine.GetGameList())
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
		if (g_UserRating)
			Engine.StartNetworkHost(playername + " (" + g_UserRating + ")");
		else
			Engine.StartNetworkHost(playername);
	}
	catch (e)
	{
		cancelSetup();
		messageBox(
			400, 200,
			sprintf(translate("Cannot host game: %(message)s."), { "message": e.message }),
			translate("Error")
		);
		return false;
	}

	startConnectionStatus("server");
	g_ServerName = servername;

	if (Engine.HasXmppClient())
		Engine.LobbySetPlayerPresence("playing");

	return true;
}

function startJoin(playername, ip)
{
	try
	{
		if (g_UserRating)
			Engine.StartNetworkJoin(playername + " (" + g_UserRating + ")", ip);
		else
			Engine.StartNetworkJoin(playername, ip);
	}
	catch (e)
	{
		cancelSetup();
		messageBox(
			400, 200,
			sprintf(translate("Cannot join game: %(message)s."), { "message": e.message }),
			translate("Error")
		);
		return false;
	}

	startConnectionStatus("client");

	if (Engine.HasXmppClient())
		Engine.LobbySetPlayerPresence("playing");
	else
	{
		// Only save the player name and host address if they're valid and we're not in the lobby
		Engine.ConfigDB_CreateValue("user", "playername.multiplayer", playername);
		Engine.ConfigDB_WriteValueToFile("user", "playername.multiplayer", playername, "config/user.cfg");
		Engine.ConfigDB_CreateValue("user", "multiplayerserver", ip);
		Engine.ConfigDB_WriteValueToFile("user", "multiplayerserver", ip, "config/user.cfg");
	}
	return true;
}

function getDefaultGameName()
{
	return sprintf(translate("%(playername)s's game"), {
		"playername": multiplayerName()
	});
}
