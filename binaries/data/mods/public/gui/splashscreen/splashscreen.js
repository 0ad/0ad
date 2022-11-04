var g_SplashScreenFile = "gui/splashscreen/splashscreen.txt";

function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.TranslateLines(Engine.ReadFile(g_SplashScreenFile));
	Engine.GetGUIObjectByName("displaySplashScreen").checked = Engine.ConfigDB_GetValue("user", "gui.splashscreen.enable") === "true";
}

function closePage()
{
	Engine.ConfigDB_CreateValue("user", "gui.splashscreen.enable", String(Engine.GetGUIObjectByName("displaySplashScreen").checked));
	Engine.ConfigDB_CreateValue("user", "gui.splashscreen.version", Engine.GetFileMTime(g_SplashScreenFile));
	Engine.ConfigDB_SaveChanges("user");
	Engine.PopGuiPage();
}
