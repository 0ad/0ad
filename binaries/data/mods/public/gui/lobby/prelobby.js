var g_LobbyIsConnecting = false;
var g_EncrytedPassword = "";
var g_PasswordInputIsHidden = false;
var g_TermsOfServiceRead = false;
var g_TermsOfUseRead = false;
var g_hasSystemMessage = false;

function init()
{
	g_EncrytedPassword = Engine.ConfigDB_GetValue("user", "lobby.password");
	if (Engine.ConfigDB_GetValue("user", "lobby.login") && g_EncrytedPassword)
		switchPage("connect");
}

function lobbyStop()
{
	Engine.GetGUIObjectByName("feedback").caption = "";

	if (g_LobbyIsConnecting == false)
		return;

	g_LobbyIsConnecting = false;
	Engine.StopXmppClient();
}

function lobbyStartConnect()
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

	feedback.caption = translate("Connecting...");
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
	if (g_LobbyIsConnecting)
		return;

	if (Engine.HasXmppClient())
		Engine.StopXmppClient();

	var account = Engine.GetGUIObjectByName("registerUsername").caption;
	var password = Engine.GetGUIObjectByName("registerPassword").caption;
	var feedback = Engine.GetGUIObjectByName("feedback");

	feedback.caption = translate("Registering...");
	g_EncrytedPassword = Engine.EncryptPassword(password, account);
	Engine.StartRegisterXmppClient(account, g_EncrytedPassword);
	g_LobbyIsConnecting = true;
	Engine.ConnectXmppClient();
}

function onTick()
{
	var pageRegisterHidden = Engine.GetGUIObjectByName("pageRegister").hidden;
	if (pageRegisterHidden)
	{
		var username = Engine.GetGUIObjectByName("connectUsername").caption;
		var password = Engine.GetGUIObjectByName("connectPassword").caption;
	}
	else
	{
		var username = Engine.GetGUIObjectByName("registerUsername").caption;
		var password = Engine.GetGUIObjectByName("registerPassword").caption;
	}
	var passwordAgain = Engine.GetGUIObjectByName("registerPasswordAgain").caption;
	var agreeTerms = Engine.GetGUIObjectByName("registerAgreeTerms");
	var feedback = Engine.GetGUIObjectByName("feedback");
	var continueButton = Engine.GetGUIObjectByName("continue");
	var sanitizedName = sanitizePlayerName(username, true, true);

	// Do not change feedback while connecting.
	if (g_LobbyIsConnecting) {}
	// Do not show feedback on the welcome screen.
	else if (!Engine.GetGUIObjectByName("pageWelcome").hidden)
	{
		feedback.caption = "";
	}
	// Check that they entered a username.
	else if (!username)
	{
		continueButton.enabled = false;
		feedback.caption = translate("Please enter your username");
	}
	// Check that they are using a valid username.
	else if (username != sanitizedName)
	{
		continueButton.enabled = false;
		feedback.caption = translate("Usernames can't contain [, ], unicode, whitespace, or commas");
	}
	// Check that they entered a password.
	else if (!password)
	{
		continueButton.enabled = false;
		feedback.caption = translate("Please enter your password");
	}
	// Allow them to connect if tests pass up to this point.
	else if (pageRegisterHidden)
	{
		if (!g_hasSystemMessage)
			feedback.caption = "";
		continueButton.enabled = true;
	}
	// Check that they entered their password again.
	else if (!passwordAgain)
	{
		continueButton.enabled = false;
		feedback.caption = translate("Please enter your password again");
	}
	// Check that the passwords match.
	else if (passwordAgain != password)
	{
		continueButton.enabled = false;
		feedback.caption = translate("Passwords do not match");
	}
	// Check that they read the Terms of Service.
	else if (!g_TermsOfServiceRead)
	{
		continueButton.enabled = false;
		feedback.caption = translate("Please read the Terms of Service");
	}
	// Check that they read the Terms of Use.
	else if (!g_TermsOfUseRead)
	{
		continueButton.enabled = false;
		feedback.caption = translate("Please read the Terms of Use");
	}
	// Check that they agree to the terms of service and use.
	else if (!agreeTerms.checked)
	{
		continueButton.enabled = false;
		feedback.caption = translate("Please agree to the Terms of Service and Terms of Use");
	}
	// Allow them to register.
	else
	{
		if (!g_hasSystemMessage)
			feedback.caption = "";
		continueButton.enabled = true;
	}

	if (!g_LobbyIsConnecting)
		// The Xmpp Client has not been created
		return;

	// The XmppClient has been created, we are waiting
	// to be connected or to receive an error.

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
			Engine.GetGUIObjectByName("connectUsername").caption = username;
			Engine.GetGUIObjectByName("connectPassword").caption = password;
			switchPage("connect");
		}
		else if(message.type == "system" && (message.level == "error" || message.text == "disconnected"))
		{
			g_hasSystemMessage = true;
			feedback.caption = toTitleCase(message.text);
			Engine.StopXmppClient();
			g_LobbyIsConnecting = false;
		}
	}
}

function switchPage(page)
{
	// First hide everything.
	if (!Engine.GetGUIObjectByName("pageWelcome").hidden)
	{
		Engine.GetGUIObjectByName("pageWelcome").hidden = true;
	}
	else if (!Engine.GetGUIObjectByName("pageRegister").hidden)
	{
		Engine.GetGUIObjectByName("pageRegister").hidden = true;
		Engine.GetGUIObjectByName("continue").hidden = true;
		var dialog = Engine.GetGUIObjectByName("dialog");
		var newSize = dialog.size;
		newSize.bottom -= 150;
		dialog.size = newSize;
	}
	else if (!Engine.GetGUIObjectByName("pageConnect").hidden)
	{
		Engine.GetGUIObjectByName("pageConnect").hidden = true;
		Engine.GetGUIObjectByName("continue").hidden = true;
	}

	// Then show appropriate page.
	switch(page)
	{
		case "welcome":
			Engine.GetGUIObjectByName("pageWelcome").hidden = false;
			break;
		case "register":
			var dialog = Engine.GetGUIObjectByName("dialog");
			var newSize = dialog.size;
			newSize.bottom += 150;
			dialog.size = newSize;
			Engine.GetGUIObjectByName("pageRegister").hidden = false;
			Engine.GetGUIObjectByName("continue").caption = translate("Register");
			Engine.GetGUIObjectByName("continue").hidden = false;
			break;
		case "connect":
			Engine.GetGUIObjectByName("pageConnect").hidden = false;
			Engine.GetGUIObjectByName("continue").caption = translate("Connect");
			Engine.GetGUIObjectByName("continue").hidden = false;
			break;
	}
}
function openTermsOfService()
{
	g_TermsOfServiceRead = true;
	Engine.PushGuiPage("page_manual.xml", {
				"page":"lobby/Terms_of_Service",
				"title":translate("Terms of Service"),
				})
}

function openTermsOfUse()
{
	g_TermsOfUseRead = true;
	Engine.PushGuiPage("page_manual.xml", {
				"page":"lobby/Terms_of_Use",
				"title":translate("Terms of Use"),
				})
}

