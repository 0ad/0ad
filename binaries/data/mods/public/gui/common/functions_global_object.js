/**
 * Update the overlay with the most recent network warning of each client.
 */
function displayGamestateNotifications()
{
	let messages = [];
	let maxTextWidth = 0;

	// Add network warnings
	if (Engine.ConfigDB_GetValue("user", "overlay.netwarnings") == "true")
	{
		let netwarnings = getNetworkWarnings();
		messages = messages.concat(netwarnings.messages);
		maxTextWidth = Math.max(maxTextWidth, netwarnings.maxTextWidth);
	}

	// Resize textbox
	let width = maxTextWidth + 20;
	let height = 14 * messages.length;

	// Position left of the dataCounter
	let top = "40";
	let right = Engine.GetGUIObjectByName("dataCounter").hidden ? "100%-15" : "100%-110";

	let bottom = top + "+" + height;
	let left = right + "-" + width;

	let gameStateNotifications = Engine.GetGUIObjectByName("gameStateNotifications");
	gameStateNotifications.caption = messages.join("\n");
	gameStateNotifications.hidden = !messages.length;
	gameStateNotifications.size = left + " " + top + " " + right + " " + bottom;

	setTimeout(displayGamestateNotifications, 1000);
}

/**
 * This function is called from the engine whenever starting a game fails.
 */
function cancelOnLoadGameError(msg)
{
	Engine.EndGame();

	if (Engine.HasXmppClient())
		Engine.StopXmppClient();

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
