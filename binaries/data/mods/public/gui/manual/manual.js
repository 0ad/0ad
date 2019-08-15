var hasCallback = false;

function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.TranslateLines(Engine.ReadFile("gui/manual/intro.txt"));
	hasCallback = data && data.callback;
}

function closeManual()
{
	if (hasCallback)
		Engine.PopGuiPageCB();
	else
		Engine.PopGuiPage();
}
