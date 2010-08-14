var g_IsConnecting = false;
var g_GameType; // "server" or "client"

function init()
{
}

function cancelSetup()
{
	if (g_IsConnecting)
		Engine.DisconnectNetworkGame();
	Engine.PopGuiPage();	
}

function startConnectionStatus(type)
{
	g_GameType = type;
	g_IsConnecting = true;
	getGUIObjectByName("connectionStatus").caption = "Connecting to server...";
}

function onTick()
{
	if (!g_IsConnecting)
		return;

	while (true)
	{
		var message = Engine.PollNetworkClient();
		if (!message)
			break;

		log("Net message: "+uneval(message));

		switch (message.type)
		{
		case "netstatus":
			switch (message.status)
			{
			case "connected":
				getGUIObjectByName("connectionStatus").caption = "Registering with server...";
				break;

			case "authenticated":
				Engine.PopGuiPage();
				Engine.PushGuiPage("page_gamesetup.xml", { "type": g_GameType });
				return; // don't process any more messages - leave them for the game GUI loop

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

function switchSetupPage(oldpage, newpage)
{
	getGUIObjectByName(oldpage).hidden = true;
	getGUIObjectByName(newpage).hidden = false;
}

function startHost(playername, servername)
{
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
	// TODO: ought to do something(?) with servername

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
	return true;
}
