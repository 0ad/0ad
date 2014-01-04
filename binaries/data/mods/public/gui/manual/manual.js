var closeCallback;

function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.ReadFile("gui/manual/" + data.page + ".txt");
	closeCallback = data.closeCallback;
}

function closeManual()
{
	Engine.PopGuiPage();
	if (closeCallback)
		closeCallback();
}
