var hasCallback = false;

function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.ReadFile("gui/manual/" + data.page + ".txt");
	if (data.callback)
		hasCallback = true;
}

function closeManual()
{
	if (hasCallback)
		Engine.PopGuiPageCB();
	else
		Engine.PopGuiPage();
}
