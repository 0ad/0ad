var g_LobbyIsConnecting = false;
var g_EncrytedPassword = "";
var g_PasswordInputIsHidden = false;

function init()
{
	g_EncrytedPassword = Engine.ConfigDB_GetValue("user", "lobby.password");
	var connectPassword = getGUIObjectByName("connectPassword");
	if (connectPassword.caption) {
		g_PasswordInputIsHidden = true;
		connectPassword.hidden = true;
		getGUIObjectByName("connectPasswordLabel").hidden = true;
		//getGUIObjectByName("nickPanel").size = "64 80 100%-32 104";
		getGUIObjectByName("nickToggle").size = "100%-64 80 100%-32 104";
	}
}

function showNickInput()
{
	getGUIObjectByName("nickToggle").hidden = true;
	getGUIObjectByName("nickPanel").hidden = false;
	if (g_PasswordInputIsHidden)
	{
		getGUIObjectByName("connectPasswordLabel").hidden = false;
		getGUIObjectByName("connectPassword").hidden = false;
	}
}

function lobbyStop()
{
	getGUIObjectByName("connectFeedback").caption = "";
	getGUIObjectByName("registerFeedback").caption = "";

	if (g_LobbyIsConnecting == false)
		return;

	g_LobbyIsConnecting = false;
	Engine.StopXmppClient();
}

function lobbyStart()
{
	if (g_LobbyIsConnecting)
		return;

	if (Engine.HasXmppClient())
		Engine.StopXmppClient();

	var username = getGUIObjectByName("connectUsername").caption;
	var password = getGUIObjectByName("connectPassword").caption;
	var feedback = getGUIObjectByName("connectFeedback");
	// Use username as nick unless overridden.
	var nickPanelHidden = getGUIObjectByName("nickPanel").hidden;
	var nick = sanitizePlayerName(nickPanelHidden ? username :
			getGUIObjectByName("joinPlayerName").caption, true, true);
	if (!username || !password)
	{
		feedback.caption = "Username or password empty";
		return;
	}

	feedback.caption = "Connecting..";
	// If they enter a different password, re-encrypt.
	if (password != g_EncrytedPassword)
		g_EncrytedPassword = Engine.EncryptPassword(password, username);
	var room = Engine.ConfigDB_GetValue("user", "lobby.room");
	Engine.StartXmppClient(username, g_EncrytedPassword, room, nick);
	g_LobbyIsConnecting = true;
	Engine.ConnectXmppClient();
}

function lobbyStartRegister()
{
	if (g_LobbyIsConnecting != false)
		return;

	if (Engine.HasXmppClient())
		Engine.StopXmppClient();

	var account = getGUIObjectByName("connectUsername").caption;
	var password = getGUIObjectByName("connectPassword").caption;
	var passwordAgain = getGUIObjectByName("registerPasswordAgain").caption;
	var feedback = getGUIObjectByName("registerFeedback");

	if (!account || !password || !passwordAgain)
	{
		feedback.caption = "Login or password empty";
		return;
	}
	if (password != passwordAgain)
	{
		feedback.caption = "Password mismatch";
		getGUIObjectByName("connectPassword").caption = "";
		getGUIObjectByName("registerPasswordAgain").caption = "";
		return;
	}
	// Check they are using a valid account name.
	sanitizedName = sanitizePlayerName(account, true, true)
	if (sanitizedName != account)
	{
		feedback.caption = "Sorry, you can't use [, ], unicode, whitespace, or commas.";
		return;
	}

	feedback.caption = "Registering...";
	if (password != g_EncrytedPassword)
		g_EncrytedPassword = Engine.EncryptPassword(password, account);
	Engine.StartRegisterXmppClient(account, g_EncrytedPassword);
	g_LobbyIsConnecting = true;
	Engine.ConnectXmppClient();
}

function onTick()
{
	if (!g_LobbyIsConnecting)
		// The Xmpp Client has not been created
		return;

	// The XmppClient has been created, we are waiting
	// to be connected or to receive an error.

	//Wake up XmppClient
	Engine.RecvXmppClient();

	//Receive messages
	while (true)
	{
		var message = Engine.LobbyGuiPollMessage();
		if (!message)
			break;

		if (message.type == "muc" && message.level == "join")
		{
			// We are connected, switch to the lobby page
			Engine.PopGuiPage();
			var username = getGUIObjectByName("connectUsername").caption;
			var password = getGUIObjectByName("connectPassword").caption;
			// Use username as nick unless overridden.
			if (getGUIObjectByName("nickPanel").hidden == true)
				var nick = sanitizePlayerName(username, true, true);
			else
				var nick = sanitizePlayerName(getGUIObjectByName("joinPlayerName").caption, true, true);

			// Switch to lobby
			Engine.SwitchGuiPage("page_lobby.xml");
			// Store nick, login, and password
			Engine.ConfigDB_CreateValue("user", "playername", nick);
			Engine.ConfigDB_CreateValue("user", "lobby.login", username);
			// We only store the encrypted password, so make sure to re-encrypt it if changed before saving.
			if (password != g_EncrytedPassword)
				g_EncrytedPassword = Engine.EncryptPassword(password, username);
			Engine.ConfigDB_CreateValue("user", "lobby.password", g_EncrytedPassword);
			Engine.ConfigDB_WriteFile("user", "config/user.cfg");

			return;
		}
		else if (message.type == "system" && message.text == "registered")
		{
			// Great, we are registered. Switch to the connection window.
			getGUIObjectByName("registerFeedback").caption = toTitleCase(message.text);
			getGUIObjectByName("connectFeedback").caption = toTitleCase(message.text);
			Engine.StopXmppClient();
			g_LobbyIsConnecting = false;
			getGUIObjectByName("pageRegister").hidden = true;
			getGUIObjectByName("pageConnect").hidden = false;
		}
		else if(message.type == "system" && (message.level == "error" || message.text == "disconnected"))
		{
			getGUIObjectByName("connectFeedback").caption = toTitleCase(message.text);
			getGUIObjectByName("registerFeedback").caption = toTitleCase(message.text);
			Engine.StopXmppClient();
			g_LobbyIsConnecting = false;
		}
	}
}
