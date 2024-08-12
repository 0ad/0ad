function messageBox(width, height, message, title, buttonCaptions)
{
	return Engine.PushGuiPage(
		"page_msgbox.xml",
		{
			"width": width,
			"height": height,
			"message": message,
			"title": title,
			"buttonCaptions": buttonCaptions
		});
}

function timedConfirmation(width, height, message, timeParameter, timeout, title, buttonCaptions)
{
	return Engine.PushGuiPage(
		"page_timedconfirmation.xml",
		{
			"width": width,
			"height": height,
			"message": message,
			"timeParameter": timeParameter,
			"timeout": timeout,
			"title": title,
			"buttonCaptions": buttonCaptions
		});
}

function openURL(url)
{
	Engine.OpenURL(url);

	messageBox(
		600, 200,
		sprintf(
			translate("Opening %(url)s\n in default web browser. Please wait…"),
			{ "url": url }
		),
		translate("Opening page"));
}
