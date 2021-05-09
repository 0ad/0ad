var g_IncompatibleModsFile = "gui/incompatible_mods/incompatible_mods.txt";

function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.TranslateLines(Engine.ReadFile(g_IncompatibleModsFile));
}

function closePage()
{
	Engine.PopGuiPage();
}
