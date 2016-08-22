// We want to pass callback functions for the different buttons in a convenient way.
// Because passing functions accross compartment boundaries is a pain, we just store them here together with some optional arguments.
// The messageBox page will return the code of the pressed button and the according function will be called.
var g_MessageBoxBtnFunctions = [];
var g_MessageBoxCallbackArgs = [];

var g_MessageBoxCallbackFunction = function(btnCode)
{
	if (btnCode !== undefined && g_MessageBoxBtnFunctions[btnCode])
	{
		// Cache the variables to make it possible to call a messageBox from a callback function.
		let callbackFunction = g_MessageBoxBtnFunctions[btnCode];
		let callbackArgs = g_MessageBoxCallbackArgs[btnCode];

		g_MessageBoxBtnFunctions = [];
		g_MessageBoxCallbackArgs = [];

		if (callbackArgs !== undefined)
			callbackFunction(callbackArgs);
		else
			callbackFunction();
		return;
	}

	g_MessageBoxBtnFunctions = [];
	g_MessageBoxCallbackArgs = [];
};

function messageBox(mbWidth, mbHeight, mbMessage, mbTitle, mbButtonCaptions, mbBtnCode, mbCallbackArgs)
{
	if (g_MessageBoxBtnFunctions && g_MessageBoxBtnFunctions.length)
	{
		warn("A messagebox was called when a previous callback function is still set, aborting!");
		return;
	}

	g_MessageBoxBtnFunctions = mbBtnCode;
	g_MessageBoxCallbackArgs = mbCallbackArgs || g_MessageBoxCallbackArgs;

	Engine.PushGuiPage("page_msgbox.xml", {
		"width": mbWidth,
		"height": mbHeight,
		"message": mbMessage,
		"title": mbTitle,
		"buttonCaptions": mbButtonCaptions,
		"callback": mbBtnCode && "g_MessageBoxCallbackFunction"
	});
}

function openURL(url)
{
	Engine.OpenURL(url);

	messageBox(
		600, 200,
		sprintf(
			translate("Opening %(url)s\n in default web browser. Please wait...."),
			{ "url": url }
		),
		translate("Opening page")
	);
}

function updateCounters()
{
	var caption = "";
	var linesCount = 0;
	var researchCount = 0;

	if (Engine.ConfigDB_GetValue("user", "overlay.fps") === "true")
	{
		caption += sprintf(translate("FPS: %(fps)4s"), { "fps": Engine.GetFPS() }) + "\n";
		++linesCount;
	}
	if (Engine.ConfigDB_GetValue("user", "overlay.realtime") === "true")
	{
		caption += (new Date()).toLocaleTimeString() + "\n";
		++linesCount;
	}

	// If game has been started
	if (typeof g_SimState != "undefined")
	{
		if (Engine.ConfigDB_GetValue("user", "gui.session.timeelapsedcounter") === "true")
		{
			var currentSpeed = Engine.GetSimRate();
			if (currentSpeed != 1.0)
				// Translation: The "x" means "times", with the mathematical meaning of multiplication.
				caption += sprintf(translate("%(time)s (%(speed)sx)"), {
					"time": timeToString(g_SimState.timeElapsed),
					"speed": Engine.FormatDecimalNumberIntoString(currentSpeed)
				});
			else
				caption += timeToString(g_SimState.timeElapsed);
			caption += "\n";
			++linesCount;
		}

		var diplomacyCeasefireCounter = Engine.GetGUIObjectByName("diplomacyCeasefireCounter");
		if (g_SimState.ceasefireActive)
		{
			// Update ceasefire counter in the diplomacy window
			var remainingTimeString = timeToString(g_SimState.ceasefireTimeRemaining);

			diplomacyCeasefireCounter.caption = sprintf(
				translateWithContext("ceasefire", "Time remaining until ceasefire is over: %(time)s."),
				{ "time": remainingTimeString }
			);

			// Update ceasefire overlay counter
			if (Engine.ConfigDB_GetValue("user", "gui.session.ceasefirecounter") === "true")
			{
				caption += remainingTimeString + "\n";
				++linesCount;
			}
		}
		else if (!diplomacyCeasefireCounter.hidden)
		{
			diplomacyCeasefireCounter.hidden = true;
			updateDiplomacy();
		}

		g_ResearchListTop = 4;
		if (linesCount)
			g_ResearchListTop += 14 * linesCount;
	}

	var dataCounter = Engine.GetGUIObjectByName("dataCounter");
	dataCounter.caption = caption;
	dataCounter.size = sprintf("100%%-100 40 100%%-5 %(bottom)s", { "bottom": 40 + 14 * linesCount });
	dataCounter.hidden = linesCount == 0;
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
 * Also called from the C++ side when ending the game.
 */
function getReplayMetadata()
{
	let extendedSimState = Engine.GuiInterfaceCall("GetExtendedSimulationState");
	return {
		"timeElapsed" : extendedSimState.timeElapsed,
		"playerStates": extendedSimState.players,
		"mapSettings": Engine.GetInitAttributes().settings
	};
}
