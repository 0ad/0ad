function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.TranslateLines(Engine.ReadFile("gui/splashscreen/" + data.page + ".txt"));
	Engine.GetGUIObjectByName("displaySplashScreen").checked = Engine.ConfigDB_GetValue("user", "gui.splashscreen.enable") === "true";
}
