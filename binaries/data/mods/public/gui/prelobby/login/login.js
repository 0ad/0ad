function init()
{
	g_LobbyMessages.connected = onLogin;

	Engine.GetGUIObjectByName("continue").caption = translate("Connect");

	// Shorten the displayed password for visual reasons only
	Engine.GetGUIObjectByName("username").caption = Engine.ConfigDB_GetValue("user", "lobby.login");
	Engine.GetGUIObjectByName("password").caption = Engine.ConfigDB_GetValue("user", "lobby.password").substr(0, 10);

	loadTermsAcceptance();
	initRememberPassword();

	updateFeedback();
}

function updateFeedback()
{
	setFeedback(checkUsername(false) || checkPassword(false) || checkTerms());
}

// Remember which user agreed to the terms
function onUsernameEdit()
{
	loadTermsAcceptance();
	updateFeedback();
}

function continueButton()
{
	setFeedback(translate("Connecting…"));

	Engine.StartXmppClient(
		Engine.GetGUIObjectByName("username").caption,
		getEncryptedPassword(),
		Engine.ConfigDB_GetValue("user", "lobby.room"),
		Engine.GetGUIObjectByName("username").caption,
		+Engine.ConfigDB_GetValue("user", "lobby.history"));

	Engine.ConnectXmppClient();
}

function onLogin(message)
{
	saveCredentials();
	saveTermsAcceptance();

	Engine.SwitchGuiPage("page_lobby.xml", {
		"dialog": false
	});
}
