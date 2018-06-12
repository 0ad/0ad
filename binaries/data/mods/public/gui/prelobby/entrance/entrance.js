function init()
{
	if (Engine.ConfigDB_GetValue("user", "lobby.login"))
		loginButton();
}

function loginButton()
{
	Engine.PushGuiPage("page_prelobby_login.xml");
}

function registerButton()
{
	Engine.PushGuiPage("page_prelobby_register.xml");
}

function cancelButton()
{
	Engine.PopGuiPage();
}
