function init()
{
	g_LobbyMessages.registered = onRegistered;

	Engine.GetGUIObjectByName("continue").caption = translate("Register");

	initRememberPassword();

	updateFeedback();
}

function updateFeedback()
{
	setFeedback(checkUsername(true) || checkPassword(true) || checkPasswordConfirmation() || checkTerms());
}

function continueButton()
{
	setFeedback(translate("Registering…"));

	Engine.StartRegisterXmppClient(
		Engine.GetGUIObjectByName("username").caption,
		getEncryptedPassword());

	Engine.ConnectXmppClient();
}

function onRegistered()
{
	saveCredentials();

	setFeedback(translate("Registered"));

	Engine.StopXmppClient();

	Engine.PopGuiPage();
	Engine.PushGuiPage("page_prelobby_login.xml");
}
