function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.TranslateLines(Engine.ReadFile("gui/splashscreen/" + data.page + ".txt"));
	Engine.GetGUIObjectByName("displaySplashScreen").checked = (Engine.ConfigDB_GetValue("user", "splashscreenversion") < Engine.GetFileMTime("gui/splashscreen/splashscreen.txt"));
}
