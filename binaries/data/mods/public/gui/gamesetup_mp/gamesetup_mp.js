/**
 * Whether we are attempting to join or host a game.
 */
var g_IsConnecting = false;

/**
 * "server" or "client"
 */
var g_GameType;

/**
 * Server title shown in the lobby gamelist.
 */
var g_ServerName = "";

/**
 * Cached to pass it to the gamesetup of the controller to report the game to the lobby.
 */
var g_ServerPort;

var g_IsRejoining = false;
var g_GameAttributes; // used when rejoining
var g_PlayerAssignments; // used when rejoining
var g_UserRating;

/**
 * Object containing the IP address and port of the STUN server.
 */
var g_StunEndpoint;

function init(attribs)
{
	g_UserRating = attribs.rating;

	switch (attribs.multiplayerGameType)
	{
	case "join":
	{
		if (Engine.HasXmppClient())
		{
			if (startJoin(attribs.name, attribs.ip, getValidPort(attribs.port), attribs.useSTUN, attribs.hostJID))
				switchSetupPage("pageConnecting");
		}
		else
			switchSetupPage("pageJoin");
		break;
	}
	case "host":
	{
		Engine.GetGUIObjectByName("hostSTUNWrapper").hidden = !Engine.HasXmppClient();
		if (Engine.HasXmppClient())
		{
			Engine.GetGUIObjectByName("hostPlayerName").caption = attribs.name;
			Engine.GetGUIObjectByName("hostServerName").caption =
				sprintf(translate("%(name)s's game"), { "name": attribs.name });

			Engine.GetGUIObjectByName("useSTUN").checked = Engine.ConfigDB_GetValue("user", "lobby.stun.enabled") == "true";
		}

		switchSetupPage("pageHost");
		break;
	}
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

	// Keep the page open if an attempt to join/host by ip failed
	if (!g_IsConnecting || (Engine.HasXmppClient() && g_GameType == "client"))
	{
		Engine.PopGuiPage();
		return;
	}

	g_IsConnecting = false;
	Engine.GetGUIObjectByName("hostFeedback").caption = "";

	if (g_GameType == "client")
		switchSetupPage("pageJoin");
	else if (g_GameType == "server")
		switchSetupPage("pageHost");
	else
		error("cancelSetup: Unrecognised multiplayer game type: " + g_GameType);
}

function confirmSetup()
{
	if (!Engine.GetGUIObjectByName("pageJoin").hidden)
	{
		let joinPlayerName = Engine.GetGUIObjectByName("joinPlayerName").caption;
		let joinServer = Engine.GetGUIObjectByName("joinServer").caption;
		let joinPort = Engine.GetGUIObjectByName("joinPort").caption;

		if (startJoin(joinPlayerName, joinServer, getValidPort(joinPort), false))
			switchSetupPage("pageConnecting");
	}
	else if (!Engine.GetGUIObjectByName("pageHost").hidden)
	{
		let hostPlayerName = Engine.GetGUIObjectByName("hostPlayerName").caption;
		let hostServerName = Engine.GetGUIObjectByName("hostServerName").caption;
		let hostPort = Engine.GetGUIObjectByName("hostPort").caption;

		if (!hostServerName)
		{
			Engine.GetGUIObjectByName("hostFeedback").caption = translate("Please enter a valid server name.");
			return;
		}

		if (getValidPort(hostPort) != +hostPort)
		{
			Engine.GetGUIObjectByName("hostFeedback").caption = sprintf(
				translate("Server port number must be between %(min)s and %(max)s."), {
					"min": g_ValidPorts.min,
					"max": g_ValidPorts.max
				});
			return;
		}

		if (startHost(hostPlayerName, hostServerName, getValidPort(hostPort)))
			switchSetupPage("pageConnecting");
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
					"isRejoining": g_IsRejoining,
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
		else
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
					Engine.SwitchGuiPage("page_gamesetup.xml", {
						"serverName": g_ServerName,
						"serverPort": g_ServerPort,
						"stunEndpoint": g_StunEndpoint
					});
					return; // don't process any more messages - leave them for the game GUI loop

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

function switchSetupPage(newPage)
{
	let multiplayerPages = Engine.GetGUIObjectByName("multiplayerPages");
	for (let page of multiplayerPages.children)
		if (page.name.startsWith("page"))
			page.hidden = true;

	if (newPage == "pageJoin" || newPage == "pageHost")
	{
		let pageSize = multiplayerPages.size;
		let halfHeight = newPage == "pageJoin" ? 130 : Engine.HasXmppClient() ? 125 : 110;
		pageSize.top = -halfHeight;
		pageSize.bottom = halfHeight;
		multiplayerPages.size = pageSize;
	}

	Engine.GetGUIObjectByName(newPage).hidden = false;

	Engine.GetGUIObjectByName("hostPlayerNameWrapper").hidden = Engine.HasXmppClient();
	Engine.GetGUIObjectByName("hostServerNameWrapper").hidden = !Engine.HasXmppClient();

	Engine.GetGUIObjectByName("continueButton").hidden = newPage == "pageConnecting";
}

function startHost(playername, servername, port)
{
	startConnectionStatus("server");

	saveSettingAndWriteToUserConfig("playername.multiplayer", playername);

	saveSettingAndWriteToUserConfig("multiplayerhosting.port", port);

	let hostFeedback = Engine.GetGUIObjectByName("hostFeedback");

	// Disallow identically named games in the multiplayer lobby
	if (Engine.HasXmppClient() &&
	    Engine.GetGameList().some(game => game.name == servername))
	{
		cancelSetup();
		hostFeedback.caption = translate("Game name already in use.");
		return false;
	}

	if (Engine.HasXmppClient() && Engine.GetGUIObjectByName("useSTUN").checked)
	{
		g_StunEndpoint = Engine.FindStunEndpoint(port);
		if (!g_StunEndpoint)
		{
			cancelSetup();
			hostFeedback.caption = translate("Failed to host via STUN.");
			return false;
		}
	}

	try
	{
		Engine.StartNetworkHost(playername + (g_UserRating ? " (" + g_UserRating + ")" : ""), port, playername);
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

	g_ServerName = servername;
	g_ServerPort = port;

	if (Engine.HasXmppClient())
		Engine.LobbySetPlayerPresence("playing");

	return true;
}

/**
 * Connects via STUN if the hostJID is given.
 */
function startJoin(playername, ip, port, useSTUN, hostJID = "")
{
	try
	{
		Engine.StartNetworkJoin(playername + (g_UserRating ? " (" + g_UserRating + ")" : ""), ip, port, useSTUN, hostJID);
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
		saveSettingAndWriteToUserConfig("playername.multiplayer", playername);
		saveSettingAndWriteToUserConfig("multiplayerserver", ip);
		saveSettingAndWriteToUserConfig("multiplayerjoining.port", port);
	}
	return true;
}

function getDefaultGameName()
{
	return sprintf(translate("%(playername)s's game"), {
		"playername": multiplayerName()
	});
}
