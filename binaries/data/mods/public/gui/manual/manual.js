var closeCallback;

function init(data)
{
	getGUIObjectByName("mainText").caption = readFile("gui/manual/" + data.page + ".txt");
	closeCallback = data.closeCallback;
}

function closeManual()
{
	Engine.PopGuiPage();
	if (closeCallback)
		closeCallback();
}
