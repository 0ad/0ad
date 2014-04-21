function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.TranslateLines(Engine.ReadFile("gui/splashscreen/" + data.page + ".txt"));
}

function openURL(url)
{
	Engine.OpenURL(url);
	messageBox(600, 200, sprintf(translate("Opening %(url)s\n in default web browser. Please wait..."), { url: url }), translate("Opening page"), 2);
}
