var g_LobbyIsConnecting = false;
var g_EncrytedPassword = "";
var g_PasswordInputIsHidden = false;

function init()
{
	g_EncrytedPassword = Engine.ConfigDB_GetValue("user", "lobby.password");
}

function lobbyStop()
{
	Engine.GetGUIObjectByName("feedback").caption = "";

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

	var username = Engine.GetGUIObjectByName("connectUsername").caption;
	var password = Engine.GetGUIObjectByName("connectPassword").caption;
	var feedback = Engine.GetGUIObjectByName("feedback");
	var room = Engine.ConfigDB_GetValue("user", "lobby.room");
	var history = Number(Engine.ConfigDB_GetValue("user", "lobby.history"));

	feedback.caption = "Connecting....";
	// If they enter a different password, re-encrypt.
	if (password != g_EncrytedPassword.substring(0, 10))
		g_EncrytedPassword = Engine.EncryptPassword(password, username);
	// We just use username as nick for simplicity.
	Engine.StartXmppClient(username, g_EncrytedPassword, room, username, history);
	g_LobbyIsConnecting = true;
	Engine.ConnectXmppClient();
}

function lobbyStartRegister()
{
	if (g_LobbyIsConnecting != false)
		return;

	if (Engine.HasXmppClient())
		Engine.StopXmppClient();

	var account = Engine.GetGUIObjectByName("connectUsername").caption;
	var password = Engine.GetGUIObjectByName("connectPassword").caption;
	var passwordAgain = Engine.GetGUIObjectByName("registerPasswordAgain").caption;
	var feedback = Engine.GetGUIObjectByName("feedback");

	// Check the passwords match.
	if (password != passwordAgain)
	{
		feedback.caption = "Passwords do not match";
		Engine.GetGUIObjectByName("connectPassword").caption = "";
		Engine.GetGUIObjectByName("registerPasswordAgain").caption = "";
		switchRegister();
		return;
	}

	feedback.caption = "Registering...";
	g_EncrytedPassword = Engine.EncryptPassword(password, account);
	Engine.StartRegisterXmppClient(account, g_EncrytedPassword);
	g_LobbyIsConnecting = true;
	Engine.ConnectXmppClient();
}

function switchRegister()
{
	if (Engine.GetGUIObjectByName("pageRegister").hidden)
	{
		lobbyStop();
		Engine.GetGUIObjectByName("pageRegister").hidden = false;
		Engine.GetGUIObjectByName("pageConnect").hidden = true;
		Engine.GetGUIObjectByName("connect").enabled = false;
	}
	else
	{
		Engine.GetGUIObjectByName("pageRegister").hidden = true;
		Engine.GetGUIObjectByName("pageConnect").hidden = false;
		Engine.GetGUIObjectByName("connect").enabled = true;
	}
}

function onTick()
{
	//
	var username = Engine.GetGUIObjectByName("connectUsername").caption;
	var password = Engine.GetGUIObjectByName("connectPassword").caption;
	var passwordAgain = Engine.GetGUIObjectByName("registerPasswordAgain").caption;
	var feedback = Engine.GetGUIObjectByName("feedback");
	var pageRegisterHidden = Engine.GetGUIObjectByName("pageRegister").hidden;
	var connectButton = Engine.GetGUIObjectByName("connect");
	var registerButton = Engine.GetGUIObjectByName("register"); 
	var sanitizedName = sanitizePlayerName(username, true, true)
	// If there aren't a username and password entered, we can't start registration or connection.
	if (!username || !password)
	{
		connectButton.enabled = false;
		registerButton.enabled = false;
		if (!username && !password)
			feedback.caption = "Please enter existing login or desired registration credentials.";
	}
	// Check they are using a valid account name.
	else if (username != sanitizedName)
	{
		feedback.caption = "Usernames can't contain [, ], unicode, whitespace, or commas.";
		connectButton.enabled = false;
		registerButton.enabled = false;
	}
	// Allow them to connect/begin registation if there aren't any problems.
	else if (pageRegisterHidden)
	{
		if (feedback.caption == "Usernames can't contain [, ], unicode, whitespace, or commas." ||
			feedback.caption == "Please enter existing login or desired registration credentials.")
			feedback.caption = "";
		connectButton.enabled = true;
		registerButton.enabled = true;
	}
	// If the password hasn't been entered again, we can't complete registation.
	if (!pageRegisterHidden && !passwordAgain)
		registerButton.enabled = false;
	else if (!pageRegisterHidden)
		registerButton.enabled = true;

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
			// Use username as nick.
			var nick = sanitizePlayerName(username, true, true);

			// Switch to lobby
			Engine.SwitchGuiPage("page_lobby.xml");
			// Store nick, login, and password
			Engine.ConfigDB_CreateValue("user", "playername", nick);
			Engine.ConfigDB_CreateValue("user", "lobby.login", username);
			// We only store the encrypted password, so make sure to re-encrypt it if changed before saving.
			if (password != g_EncrytedPassword.substring(0, 10))
				g_EncrytedPassword = Engine.EncryptPassword(password, username);
			Engine.ConfigDB_CreateValue("user", "lobby.password", g_EncrytedPassword);
			Engine.ConfigDB_WriteFile("user", "config/user.cfg");

			return;
		}
		else if (message.type == "system" && message.text == "registered")
		{
			// Great, we are registered. Switch to the connection window.
			feedback.caption = toTitleCase(message.text);
			Engine.StopXmppClient();
			g_LobbyIsConnecting = false;
			switchRegister();
		}
		else if(message.type == "system" && (message.level == "error" || message.text == "disconnected"))
		{
			feedback.caption = toTitleCase(message.text);
			Engine.StopXmppClient();
			g_LobbyIsConnecting = false;
		}
	}
}
