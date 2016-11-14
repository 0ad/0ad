function cancelOnError(msg)
{
	Engine.EndGame();

	Engine.SwitchGuiPage("page_pregame.xml");

	if (msg)
		Engine.PushGuiPage("page_msgbox.xml", {
			"width": 500,
			"height": 200,
			"message": '[font="sans-bold-18"]' + msg + '[/font]',
			"title": translate("Loading Aborted"),
			"mode": 2
		});

	Engine.ResetCursor();
}
