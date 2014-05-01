var hasCallback = false;

function init(data)
{
	Engine.GetGUIObjectByName("mainText").caption = Engine.TranslateLines(Engine.ReadFile("gui/" + data.page + ".txt"));
	if (data.callback)
		hasCallback = true;
	if (data.title)
		Engine.GetGUIObjectByName("title").caption = data.title;
	if (data.url)
	{
		var urlButton = Engine.GetGUIObjectByName("url");
		var callback = function(url)
		{
			return function()
				openURL(url);
		}(data.url)
		urlButton.onPress = callback;
		urlButton.hidden = false;
	}
}

function closeManual()
{
	if (hasCallback)
		Engine.PopGuiPageCB();
	else
		Engine.PopGuiPage();
}
