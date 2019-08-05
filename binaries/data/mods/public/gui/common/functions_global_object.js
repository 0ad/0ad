function updateCounters()
{
	let counters = [];

	if (Engine.ConfigDB_GetValue("user", "overlay.fps") === "true")
		// dennis-ignore: *
		counters.push(sprintf(translate("FPS: %(fps)4s"), { "fps": Engine.GetFPS() }));

	if (Engine.ConfigDB_GetValue("user", "overlay.realtime") === "true")
		counters.push((new Date()).toLocaleTimeString());

	// If game has been started
	if (typeof appendSessionCounters != "undefined")
		appendSessionCounters(counters);

	let dataCounter = Engine.GetGUIObjectByName("dataCounter");
	dataCounter.caption = counters.join("\n") + "\n";
	dataCounter.hidden = !counters.length;
	dataCounter.size = sprintf("%(left)s %(top)s %(right)s %(bottom)s", {
		"left": "100%%-100",
		"top": "40",
		"right": "100%%-5",
		"bottom": 40 + 14 * counters.length
	});
}

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

/**
 * Also called from the C++ side when ending the game.
 * The current page can be the summary screen or a message box, so it can't be moved to session/.
 */
function getReplayMetadata()
{
	let extendedSimState = Engine.GuiInterfaceCall("GetExtendedSimulationState");
	return {
		"timeElapsed": extendedSimState.timeElapsed,
		"playerStates": extendedSimState.players,
		"mapSettings": Engine.GetInitAttributes().settings
	};
}
