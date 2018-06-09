var g_LobbyIsConnecting = false;
var g_EncryptedPassword = "";
var g_PasswordInputIsHidden = false;
var g_DisplayingSystemMessage = false;

var g_Terms = {
	"Service": {
		"title": translate("Terms of Service"),
		"instruction": translate("Please read the Terms of Service"),
		"file": "prelobby/Terms_of_Service",
		"read": false
	},
	"Use": {
		"title": translate("Terms of Use"),
		"instruction": translate("Please read the Terms of Use"),
		"file": "prelobby/Terms_of_Use",
		"read": false
	}
};

function init()
{
	let username = Engine.ConfigDB_GetValue("user", "lobby.login");
	g_EncryptedPassword = Engine.ConfigDB_GetValue("user", "lobby.password");

	// We only show 10 characters to make it look decent.
	Engine.GetGUIObjectByName("connectUsername").caption = username;
	Engine.GetGUIObjectByName("connectPassword").caption = g_EncryptedPassword.substring(0, 10);

	Engine.GetGUIObjectByName("rememberPassword").checked =
		Engine.ConfigDB_GetValue("user", "lobby.rememberpassword") == "true";

	if (username && g_EncryptedPassword)
		switchPage("connect");
}

function lobbyStop()
{
	Engine.GetGUIObjectByName("feedback").caption = "";

	if (!g_LobbyIsConnecting)
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

	Engine.GetGUIObjectByName("continue").enabled = false;
	let username = Engine.GetGUIObjectByName("connectUsername").caption;
	let password = Engine.GetGUIObjectByName("connectPassword").caption;
	let feedback = Engine.GetGUIObjectByName("feedback");
	let room = Engine.ConfigDB_GetValue("user", "lobby.room");
	let history = Number(Engine.ConfigDB_GetValue("user", "lobby.history"));

	feedback.caption = translate("Connecting…");
	// If they enter a different password, re-encrypt.
	if (password != g_EncryptedPassword.substring(0, 10))
		g_EncryptedPassword = Engine.EncryptPassword(password, username);
	// We just use username as nick for simplicity.
	Engine.StartXmppClient(username, g_EncryptedPassword, room, username, history);
	g_LobbyIsConnecting = true;
	Engine.ConnectXmppClient();
}

function lobbyStartRegister()
{
	if (g_LobbyIsConnecting)
		return;

	if (Engine.HasXmppClient())
		Engine.StopXmppClient();

	Engine.GetGUIObjectByName("continue").enabled = false;
	let account = Engine.GetGUIObjectByName("registerUsername").caption;
	let password = Engine.GetGUIObjectByName("registerPassword").caption;
	let feedback = Engine.GetGUIObjectByName("feedback");

	feedback.caption = translate("Registering…");
	g_EncryptedPassword = Engine.EncryptPassword(password, account);
	Engine.StartRegisterXmppClient(account, g_EncryptedPassword);
	g_LobbyIsConnecting = true;
	Engine.ConnectXmppClient();
}

function onTick()
{
	let pageRegisterHidden = Engine.GetGUIObjectByName("pageRegister").hidden;
	let username = Engine.GetGUIObjectByName(pageRegisterHidden ? "connectUsername" : "registerUsername").caption;
	let password = Engine.GetGUIObjectByName(pageRegisterHidden ? "connectPassword" : "registerPassword").caption;
	let passwordAgain = Engine.GetGUIObjectByName("registerPasswordAgain").caption;

	let agreeTerms = Engine.GetGUIObjectByName("registerAgreeTerms");
	let feedback = Engine.GetGUIObjectByName("feedback");
	let continueButton = Engine.GetGUIObjectByName("continue");

	// Do not change feedback while connecting.
	if (g_LobbyIsConnecting) {}
	// Do not show feedback on the welcome screen.
	else if (!Engine.GetGUIObjectByName("pageWelcome").hidden)
	{
		feedback.caption = "";
		g_DisplayingSystemMessage = false;
	}
	// Check that they entered a username.
	else if (!username)
	{
		continueButton.enabled = false;
		feedback.caption = translate("Please enter your username");
	}
	// Prevent registation (but not login) with non-alphanumerical characters
	else if (!pageRegisterHidden && (!username.match(/^[a-z0-9._-]*$/i) || username.length > 20))
	{
		continueButton.enabled = false;
		feedback.caption = translate("Invalid username");
	}
	// Check that they entered a password.
	else if (!password)
	{
		continueButton.enabled = false;
		feedback.caption = pageRegisterHidden ?
			translateWithContext("login", "Please enter your password") :
			translateWithContext("register", "Please enter your password");
	}
	// Allow them to connect if tests pass up to this point.
	else if (pageRegisterHidden)
	{
		if (!g_DisplayingSystemMessage)
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
	else if (!g_Terms.Service.read)
	{
		continueButton.enabled = false;
		feedback.caption = g_Terms.Service.instruction;
	}
	// Check that they read the Terms of Use.
	else if (!g_Terms.Use.read)
	{
		continueButton.enabled = false;
		feedback.caption = g_Terms.Use.instruction;
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
		if (!g_DisplayingSystemMessage)
			feedback.caption = "";
		continueButton.enabled = true;
	}

	// Handle queued messages from the XMPP client (if running and if any)
	let message;
	while ((message = Engine.LobbyGuiPollNewMessage()) != undefined)
	{
		// TODO: Properly deal with unrecognized messages
		if (message.type != "system" || !message.level)
			continue;

		g_LobbyIsConnecting = false;

		switch (message.level)
		{
		case "error":
		{
			Engine.GetGUIObjectByName("feedback").caption = message.text;
			g_DisplayingSystemMessage = true;
			Engine.StopXmppClient();
			break;
		}
		case "disconnected":
		{
			Engine.GetGUIObjectByName("feedback").caption = message.reason ||
				translate("Unknown error. This usually occurs because the same IP address is not allowed to register more than one account within one hour.");
			g_DisplayingSystemMessage = true;
			Engine.StopXmppClient();
			break;
		}
		case "registered":
			Engine.GetGUIObjectByName("feedback").caption = translate("Registered");
			g_DisplayingSystemMessage = true;
			Engine.GetGUIObjectByName("connectUsername").caption = username;
			Engine.GetGUIObjectByName("connectPassword").caption = password;
			Engine.StopXmppClient();
			switchPage("connect");
			break;
		case "connected":
		{
			Engine.PopGuiPage();
			Engine.SwitchGuiPage("page_lobby.xml", { "dialog": false });
			saveSettingAndWriteToUserConfig("playername.multiplayer", username);
			saveSettingAndWriteToUserConfig("lobby.login", username);
			// We only store the encrypted password, so make sure to re-encrypt it if changed before saving.
			if (password != g_EncryptedPassword.substring(0, 10))
				g_EncryptedPassword = Engine.EncryptPassword(password, username);
			Engine.ConfigDB_CreateValue("user", "lobby.password", g_EncryptedPassword);
			if (Engine.ConfigDB_GetValue("user", "lobby.rememberpassword") == "true")
				Engine.ConfigDB_WriteValueToFile("user", "lobby.password", g_EncryptedPassword, "config/user.cfg");
			else
			{
				Engine.ConfigDB_RemoveValue("user", "lobby.password");
				Engine.ConfigDB_WriteFile("user", "config/user.cfg");
			}
			break;
		}
		}
	}
}

function switchPage(page)
{
	// First hide everything.
	if (!Engine.GetGUIObjectByName("pageWelcome").hidden)
		Engine.GetGUIObjectByName("pageWelcome").hidden = true;
	else if (!Engine.GetGUIObjectByName("pageRegister").hidden)
	{
		Engine.GetGUIObjectByName("pageRegister").hidden = true;
		Engine.GetGUIObjectByName("continue").hidden = true;
		let dialog = Engine.GetGUIObjectByName("dialog");
		let newSize = dialog.size;
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
	{
		let dialog = Engine.GetGUIObjectByName("dialog");
		let newSize = dialog.size;
		newSize.bottom += 150;
		dialog.size = newSize;

		Engine.GetGUIObjectByName("pageRegister").hidden = false;
		Engine.GetGUIObjectByName("continue").caption = translate("Register");
		Engine.GetGUIObjectByName("continue").hidden = false;
		break;
	}
	case "connect":
		Engine.GetGUIObjectByName("pageConnect").hidden = false;
		Engine.GetGUIObjectByName("continue").caption = translate("Connect");
		Engine.GetGUIObjectByName("continue").hidden = false;
		break;
	}
}

function openTerms(terms)
{
	g_Terms[terms].read = true;

	Engine.GetGUIObjectByName("registerAgreeTerms").enabled = g_Terms.Service.read && g_Terms.Use.read;

	Engine.PushGuiPage("page_manual.xml", {
		"page": g_Terms[terms].file,
		"title": g_Terms[terms].title
	});
}

function prelobbyConnect()
{
	if (!Engine.GetGUIObjectByName("pageConnect").hidden)
		lobbyStartConnect();
	else if (!Engine.GetGUIObjectByName("pageRegister").hidden)
		lobbyStartRegister();
}

function prelobbyCancel()
{
	lobbyStop();

	Engine.GetGUIObjectByName("feedback").caption = "";

	if (Engine.GetGUIObjectByName("pageWelcome").hidden)
		switchPage("welcome");
	else
		Engine.PopGuiPage();
}

function toggleRememberPassword()
{
	let checkbox = Engine.GetGUIObjectByName("rememberPassword");
	let enabled = Engine.ConfigDB_GetValue("user", "lobby.rememberpassword") == "true";
	if (!checkbox.checked && enabled && Engine.ConfigDB_GetValue("user", "lobby.password"))
	{
		messageBox(
			360, 160,
			translate("Are you sure you want to delete the password after connecting?"),
			translate("Confirmation"),
			[translate("No"), translate("Yes")],
			[function() { checkbox.checked = true; },
			 function() { saveSettingAndWriteToUserConfig("lobby.rememberpassword", String(!enabled)); }]
		);
	}
	else
		saveSettingAndWriteToUserConfig("lobby.rememberpassword", String(!enabled));
}
