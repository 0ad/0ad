// Menu / panel border size
const MARGIN = 4;

// Includes the main menu button
const NUM_BUTTONS = 9;

// Regular menu buttons
const BUTTON_HEIGHT = 32;

// The position where the bottom of the menu will end up (currently 228)
const END_MENU_POSITION = (BUTTON_HEIGHT * NUM_BUTTONS) + MARGIN;

// Menu starting position: bottom
const MENU_BOTTOM = 0;

// Menu starting position: top
const MENU_TOP = MENU_BOTTOM - END_MENU_POSITION;

// Menu starting position: overall
const INITIAL_MENU_POSITION = "100%-164 " + MENU_TOP + " 100% " + MENU_BOTTOM;

// Number of pixels per millisecond to move
const MENU_SPEED = 1.2;

// Available resources in trade and tribute menu
const RESOURCES = ["food", "wood", "stone", "metal"];

// Trade menu: step for probability changes
const STEP = 5;

// Shown in the trade dialog.
const g_IdleTraderTextColor = "orange";

var g_IsMenuOpen = false;

var g_IsDiplomacyOpen = false;
var g_IsTradeOpen = false;

// Redefined every time someone makes a tribute (so we can save some data in a closure). Called in input.js handleInputBeforeGui.
var g_FlushTributing = function() {};

// Ignore size defined in XML and set the actual menu size here
function initMenuPosition()
{
	Engine.GetGUIObjectByName("menu").size = INITIAL_MENU_POSITION;
}

function updateMenuPosition(dt)
{
	let menu = Engine.GetGUIObjectByName("menu");

	let maxOffset = g_IsMenuOpen ? (END_MENU_POSITION - menu.size.bottom) : (menu.size.top - MENU_TOP);
	if (maxOffset <= 0)
		return;

	let offset = Math.min(MENU_SPEED * dt, maxOffset) * (g_IsMenuOpen ? +1 : -1);

	let size = menu.size;
	size.top += offset;
	size.bottom += offset;
	menu.size = size;
}

// Opens the menu by revealing the screen which contains the menu
function openMenu()
{
	g_IsMenuOpen = true;
}

// Closes the menu and resets position
function closeMenu()
{
	g_IsMenuOpen = false;
}

function toggleMenu()
{
	g_IsMenuOpen = !g_IsMenuOpen;
}

function optionsMenuButton()
{
	closeOpenDialogs();
	openOptions();
}

function chatMenuButton()
{
	closeOpenDialogs();
	openChat();
}

function diplomacyMenuButton()
{
	closeOpenDialogs();
	openDiplomacy();
}

function pauseMenuButton()
{
	togglePause();
}

function resignMenuButton()
{
	closeOpenDialogs();
	pauseGame();

	messageBox(
		400, 200,
		translate("Are you sure you want to resign?"),
		translate("Confirmation"),
		0,
		[translate("No"), translate("Yes")],
		[resumeGame, resignGame]
	);
}

function exitMenuButton()
{
	closeOpenDialogs();
	pauseGame();

	let messageTypes = {
		"host": {
			"caption": translate("Are you sure you want to quit? Leaving will disconnect all other players."),
			"buttons": [resumeGame, leaveGame]
		},
		"client": {
			"caption": translate("Are you sure you want to quit?"),
			"buttons": [resumeGame, resignQuestion]
		},
		"singleplayer": {
			"caption": translate("Are you sure you want to quit?"),
			"buttons": [resumeGame, leaveGame]
		}
	};

	let messageType = g_IsNetworked && g_IsController ? "host" :
		(g_IsNetworked && !g_IsObserver ? "client" : "singleplayer");

	messageBox(
		400, 200,
		messageTypes[messageType].caption,
		translate("Confirmation"),
		0,
		[translate("No"), translate("Yes")],
		messageTypes[messageType].buttons
	);
}

function resignQuestion()
{
	messageBox(
		400, 200,
		translate("Do you want to resign or will you return soon?"),
		translate("Confirmation"),
		0,
		[translate("I will return"), translate("I resign")],
		[leaveGame, resignGame],
		[true, false]
	);
}

function openDeleteDialog(selection)
{
	closeOpenDialogs();

	let deleteSelectedEntities = function (selectionArg)
	{
		Engine.PostNetworkCommand({ "type": "delete-entities", "entities": selectionArg });
	};

	messageBox(
		400, 200,
		translate("Destroy everything currently selected?"),
		translate("Delete"),
		0,
		[translate("No"), translate("Yes")],
		[resumeGame, deleteSelectedEntities],
		[null, selection]
	);
}

function openSave()
{
	closeOpenDialogs();
	pauseGame();
	Engine.PushGuiPage("page_savegame.xml", { "savedGameData": getSavedGameData(), "callback": "resumeGame" });
}

function openOptions()
{
	closeOpenDialogs();
	pauseGame();
	Engine.PushGuiPage("page_options.xml", { "callback": "resumeGame" });
}

function openChat()
{
	if (g_Disconnected)
		return;

	closeOpenDialogs();

	setTeamChat(false);

	Engine.GetGUIObjectByName("chatInput").focus(); // Grant focus to the input area
	Engine.GetGUIObjectByName("chatDialogPanel").hidden = false;
}

function closeChat()
{
	Engine.GetGUIObjectByName("chatInput").caption = ""; // Clear chat input
	Engine.GetGUIObjectByName("chatInput").blur(); // Remove focus
	Engine.GetGUIObjectByName("chatDialogPanel").hidden = true;
}

/**
 * If the teamchat hotkey was pressed, set allies or observers as addressees,
 * otherwise send to everyone.
 */
function setTeamChat(teamChat = false)
{
	let command = teamChat ? (g_IsObserver ? "/observers" : "/allies") : "";
	let chatAddressee = Engine.GetGUIObjectByName("chatAddressee");
	chatAddressee.selected = chatAddressee.list_data.indexOf(command);
}
/**
 * Opens chat-window or closes it and sends the userinput.
 */
function toggleChatWindow(teamChat)
{
	if (g_Disconnected)
		return;

	let chatWindow = Engine.GetGUIObjectByName("chatDialogPanel");
	let chatInput = Engine.GetGUIObjectByName("chatInput");
	let hidden = chatWindow.hidden;

	closeOpenDialogs();

	if (hidden)
	{
		setTeamChat(teamChat);
		chatInput.focus();
	}
	else
	{
		if (chatInput.caption.length)
		{
			submitChatInput();
			return;
		}
		chatInput.caption = "";
	}

	chatWindow.hidden = !hidden;
}

function openDiplomacy()
{
	closeOpenDialogs();

	if (g_ViewedPlayer < 1)
		return;

	g_IsDiplomacyOpen = true;

	let isCeasefireActive = GetSimState().ceasefireActive;

	// Get offset for one line
	let onesize = Engine.GetGUIObjectByName("diplomacyPlayer[0]").size;
	let rowsize = onesize.bottom - onesize.top;

	// We don't include gaia
	for (let i = 1; i < g_Players.length; ++i)
	{
		let myself = i == g_ViewedPlayer;
		let playerInactive = isPlayerObserver(g_ViewedPlayer) || isPlayerObserver(i);
		let hasAllies = g_Players.filter(player => player.isMutualAlly[g_ViewedPlayer]).length > 1;

		diplomacySetupTexts(i, rowsize);
		diplomacyFormatStanceButtons(i, myself || playerInactive || isCeasefireActive || g_Players[g_ViewedPlayer].teamsLocked);
		diplomacyFormatTributeButtons(i, myself || playerInactive);
		diplomacyFormatAttackRequestButton(i, myself || playerInactive || isCeasefireActive || !hasAllies || !g_Players[i].isEnemy[g_ViewedPlayer]);
	}

	Engine.GetGUIObjectByName("diplomacyDialogPanel").hidden = false;
}

function diplomacySetupTexts(i, rowsize)
{
	// Apply offset
	let row = Engine.GetGUIObjectByName("diplomacyPlayer["+(i-1)+"]");
	let size = row.size;
	size.top = rowsize * (i-1);
	size.bottom = rowsize * i;
	row.size = size;

	// Set background color
	row.sprite = "color: " + rgbToGuiColor(g_Players[i].color) + " 32";

	Engine.GetGUIObjectByName("diplomacyPlayerName["+(i-1)+"]").caption = colorizePlayernameByID(i);
	Engine.GetGUIObjectByName("diplomacyPlayerCiv["+(i-1)+"]").caption = g_CivData[g_Players[i].civ].Name;

	Engine.GetGUIObjectByName("diplomacyPlayerTeam["+(i-1)+"]").caption =
		g_Players[i].team < 0 ? translateWithContext("team", "None") : g_Players[i].team+1;

	Engine.GetGUIObjectByName("diplomacyPlayerTheirs["+(i-1)+"]").caption =
		i == g_ViewedPlayer ? "" :
		g_Players[i].isAlly[g_ViewedPlayer] ? translate("Ally") :
		g_Players[i].isNeutral[g_ViewedPlayer] ? translate("Neutral") : translate("Enemy");
}

function diplomacyFormatStanceButtons(i, hidden)
{
	for (let stance of ["Ally", "Neutral", "Enemy"])
	{
		let button = Engine.GetGUIObjectByName("diplomacyPlayer"+stance+"["+(i-1)+"]");
		button.hidden = hidden;
		if (hidden)
			continue;

		button.caption = g_Players[g_ViewedPlayer]["is" + stance][i] ? translate("x") : "";
		button.enabled = controlsPlayer(g_ViewedPlayer);
		button.onpress = (function(player, stance) { return function() {
			Engine.PostNetworkCommand({ "type": "diplomacy", "player": i, "to": stance.toLowerCase() });
		}; })(i, stance);
	}
}

function diplomacyFormatTributeButtons(i, hidden)
{
	for (let resource of RESOURCES)
	{
		let button = Engine.GetGUIObjectByName("diplomacyPlayerTribute"+resource[0].toUpperCase()+resource.substring(1)+"["+(i-1)+"]");
		button.hidden = hidden;
		if (hidden)
			continue;

		button.enabled = controlsPlayer(g_ViewedPlayer);
		button.tooltip = formatTributeTooltip(g_Players[i], resource, 100);
		button.onpress = (function(i, resource, button) {
			// Shift+click to send 500, shift+click+click to send 1000, etc.
			// See INPUT_MASSTRIBUTING in input.js
			let multiplier = 1;
			return function() {
				let isBatchTrainPressed = Engine.HotkeyIsPressed("session.masstribute");
				if (isBatchTrainPressed)
				{
					inputState = INPUT_MASSTRIBUTING;
					multiplier += multiplier == 1 ? 4 : 5;
				}

				let amounts = {};
				for (let type of RESOURCES)
					amounts[type] = 0;
				amounts[resource] = 100 * multiplier;

				button.tooltip = formatTributeTooltip(g_Players[i], resource, amounts[resource]);

				// This is in a closure so that we have access to `player`, `amounts`, and `multiplier` without some
				// evil global variable hackery.
				g_FlushTributing = function() {
					Engine.PostNetworkCommand({ "type": "tribute", "player": i, "amounts":  amounts });
					multiplier = 1;
					button.tooltip = formatTributeTooltip(g_Players[i], resource, 100);
				};

				if (!isBatchTrainPressed)
					g_FlushTributing();
			};
		})(i, resource, button);
	}
}

function diplomacyFormatAttackRequestButton(i, hidden)
{
	let button = Engine.GetGUIObjectByName("diplomacyAttackRequest["+(i-1)+"]");
	button.hidden = hidden;
	if (hidden)
		return;

	button.enabled = controlsPlayer(g_ViewedPlayer);
	button.tooltip = translate("Request your allies to attack this enemy");
	button.onpress = (function(i) { return function() {
		Engine.PostNetworkCommand({ "type": "attack-request", "source": g_ViewedPlayer, "target": i });
	}; })(i);
}

function closeDiplomacy()
{
	g_IsDiplomacyOpen = false;
	Engine.GetGUIObjectByName("diplomacyDialogPanel").hidden = true;
}

function toggleDiplomacy()
{
	let open = g_IsDiplomacyOpen;
	closeOpenDialogs();

	if (!open)
		openDiplomacy();
}

function openTrade()
{
	closeOpenDialogs();

	if (g_ViewedPlayer < 1)
		return;

	g_IsTradeOpen = true;

	var updateButtons = function()
	{
		for (var res in button)
		{
			button[res].label.caption = proba[res] + "%";

			button[res].sel.hidden = !controlsPlayer(g_ViewedPlayer) || res != selec;
			button[res].up.hidden = !controlsPlayer(g_ViewedPlayer) || res == selec || proba[res] == 100 || proba[selec] == 0;
			button[res].dn.hidden = !controlsPlayer(g_ViewedPlayer) || res == selec || proba[res] == 0 || proba[selec] == 100;
		}
	};

	var proba = Engine.GuiInterfaceCall("GetTradingGoods", g_ViewedPlayer);
	var button = {};
	var selec = RESOURCES[0];
	for (var i = 0; i < RESOURCES.length; ++i)
	{
		var buttonResource = Engine.GetGUIObjectByName("tradeResource["+i+"]");
		if (i > 0)
		{
			var size = Engine.GetGUIObjectByName("tradeResource["+(i-1)+"]").size;
			var width = size.right - size.left;
			size.left += width;
			size.right += width;
			Engine.GetGUIObjectByName("tradeResource["+i+"]").size = size;
		}
		var resource = RESOURCES[i];
		proba[resource] = (proba[resource] ? proba[resource] : 0);
		var buttonResource = Engine.GetGUIObjectByName("tradeResourceButton["+i+"]");
		var icon = Engine.GetGUIObjectByName("tradeResourceIcon["+i+"]");
		icon.sprite = "stretched:session/icons/resources/" + resource + ".png";
		var label = Engine.GetGUIObjectByName("tradeResourceText["+i+"]");
		var buttonUp = Engine.GetGUIObjectByName("tradeArrowUp["+i+"]");
		var buttonDn = Engine.GetGUIObjectByName("tradeArrowDn["+i+"]");
		var iconSel = Engine.GetGUIObjectByName("tradeResourceSelection["+i+"]");
		button[resource] = { "up": buttonUp, "dn": buttonDn, "label": label, "sel": iconSel };

		buttonResource.enabled = controlsPlayer(g_ViewedPlayer);
		buttonResource.onpress = (function(resource){
			return function() {
				if (Engine.HotkeyIsPressed("session.fulltradeswap"))
				{
					for (var ress of RESOURCES)
						proba[ress] = 0;
					proba[resource] = 100;
					Engine.PostNetworkCommand({"type": "set-trading-goods", "tradingGoods": proba});
				}
				selec = resource;
				updateButtons();
			};
		})(resource);

		buttonUp.enabled = controlsPlayer(g_ViewedPlayer);
		buttonUp.onpress = (function(resource){
			return function() {
				proba[resource] += Math.min(STEP, proba[selec]);
				proba[selec]    -= Math.min(STEP, proba[selec]);
				Engine.PostNetworkCommand({"type": "set-trading-goods", "tradingGoods": proba});
				updateButtons();
			};
		})(resource);

		buttonDn.enabled = controlsPlayer(g_ViewedPlayer);
		buttonDn.onpress = (function(resource){
			return function() {
				proba[selec]    += Math.min(STEP, proba[resource]);
				proba[resource] -= Math.min(STEP, proba[resource]);
				Engine.PostNetworkCommand({"type": "set-trading-goods", "tradingGoods": proba});
				updateButtons();
			};
		})(resource);
	}
	updateButtons();

	let traderNumber = Engine.GuiInterfaceCall("GetTraderNumber", g_ViewedPlayer);
	Engine.GetGUIObjectByName("landTraders").caption = getIdleLandTradersText(traderNumber);
	Engine.GetGUIObjectByName("shipTraders").caption = getIdleShipTradersText(traderNumber);

	Engine.GetGUIObjectByName("tradeDialogPanel").hidden = false;
}

function getIdleLandTradersText(traderNumber)
{
	let active = traderNumber.landTrader.trading;
	let garrisoned = traderNumber.landTrader.garrisoned;
	let inactive = traderNumber.landTrader.total - active - garrisoned;

	let messageTypes = {
		"active": {
			"garrisoned": {
				"no-inactive": translate("%(openingTradingString)s, and %(garrisonedString)s."),
				"inactive":    translate("%(openingTradingString)s, %(garrisonedString)s, and %(inactiveString)s.")
			},
			"no-garrisoned": {
				"no-inactive": translate("%(openingTradingString)s."),
				"inactive":    translate("%(openingTradingString)s, and %(inactiveString)s.")
			}
		},
		"no-active": {
			"garrisoned": {
				"no-inactive": translate("%(openingGarrisonedString)s."),
				"inactive": translate("%(openingGarrisonedString)s, and %(inactiveString)s.")
			},
			"no-garrisoned": {
				"inactive": translatePlural("There is %(inactiveString)s.", "There are %(inactiveString)s.", inactive),
				"no-inactive": translate("There are no land traders.")
			}
		}
	};

	let message = messageTypes[active ? "active" : "no-active"][garrisoned ? "garrisoned" : "no-garrisoned"][inactive ? "inactive" : "no-inactive"];

	let activeString = sprintf(
		translatePlural(
			"There is %(numberTrading)s land trader trading",
			"There are %(numberTrading)s land traders trading",
			active
		),
		{ "numberTrading": active }
	);

	let inactiveString = sprintf(active || garrisoned ?
		translatePlural(
			"%(numberOfLandTraders)s inactive",
			"%(numberOfLandTraders)s inactive",
			inactive
		) :
		translatePlural(
			"%(numberOfLandTraders)s land trader inactive",
			"%(numberOfLandTraders)s land traders inactive",
			inactive
		),
		{ "numberOfLandTraders": inactive }
	);

	let garrisonedString = sprintf(active || inactive ?
		translatePlural(
			"%(numberGarrisoned)s garrisoned on a trading merchant ship",
			"%(numberGarrisoned)s garrisoned on a trading merchant ship",
			garrisoned
		) :
		translatePlural(
			"There is %(numberGarrisoned)s land trader garrisoned on a trading merchant ship",
			"There are %(numberGarrisoned)s land traders garrisoned on a trading merchant ship",
			garrisoned
		),
		{ "numberGarrisoned": garrisoned }
	);

	return sprintf(message, {
		"openingTradingString": activeString,
		"openingGarrisonedString": garrisonedString,
		"garrisonedString": garrisonedString,
		"inactiveString": "[color=\"" + g_IdleTraderTextColor + "\"]" + inactiveString + "[/color]"
	});
}

function getIdleShipTradersText(traderNumber)
{
	let active = traderNumber.shipTrader.trading;
	let inactive = traderNumber.shipTrader.total - active;

	let messageTypes = {
		"active": {
			"inactive": translate("%(openingTradingString)s, and %(inactiveString)s."),
			"no-inactive": translate("%(openingTradingString)s.")
		},
		"no-active": {
			"inactive": translatePlural("There is %(inactiveString)s.", "There are %(inactiveString)s.", inactive),
			"no-inactive": translate("There are no merchant ships.")
		}
	};

	let message = messageTypes[active ? "active" : "no-active"][inactive ? "inactive" : "no-inactive"];

	let activeString = sprintf(
		translatePlural(
			"There is %(numberTrading)s merchant ship trading",
			"There are %(numberTrading)s merchant ships trading",
			active
		),
		{ "numberTrading": active }
	);

	let inactiveString = sprintf(active ?
		translatePlural(
			"%(numberOfShipTraders)s inactive",
			"%(numberOfShipTraders)s inactive",
			inactive
		) :
		translatePlural(
			"%(numberOfShipTraders)s merchant ship inactive",
			"%(numberOfShipTraders)s merchant ships inactive",
			inactive
		),
		{ "numberOfShipTraders": inactive }
	);

	return sprintf(message, {
		"openingTradingString": activeString,
		"inactiveString": "[color=\"" + g_IdleTraderTextColor + "\"]" + inactiveString + "[/color]"
	});
}

function closeTrade()
{
	g_IsTradeOpen = false;
	Engine.GetGUIObjectByName("tradeDialogPanel").hidden = true;
}

function toggleTrade()
{
	let open = g_IsTradeOpen;
	closeOpenDialogs();

	if (!open)
		openTrade();
}

function toggleGameSpeed()
{
	let gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
	gameSpeed.hidden = !gameSpeed.hidden;
}

function openGameSummary()
{
	closeOpenDialogs();
	pauseGame();

	let extendedSimState = Engine.GuiInterfaceCall("GetExtendedSimulationState");

	Engine.PushGuiPage("page_summary.xml", {
		"timeElapsed" : extendedSimState.timeElapsed,
		"playerStates": extendedSimState.players,
		"players": g_Players,
		"mapSettings": Engine.GetMapSettings(),
		"isInGame": true,
		"gameResult": translate("Current Scores"),
		"callback": "resumeGame"
	});
}

function openStrucTree()
{
	closeOpenDialogs();
	pauseGame();

	// TODO add info about researched techs and unlocked entities
	Engine.PushGuiPage("page_structree.xml", {
		"civ" : g_Players[g_ViewedPlayer].civ,
		"callback": "resumeGame",
	});
}

/**
 * Pause the game in single player mode.
 */
function pauseGame()
{
	if (g_IsNetworked)
		return;

	Engine.GetGUIObjectByName("pauseButtonText").caption = translate("Resume");
	Engine.GetGUIObjectByName("pauseOverlay").hidden = false;
	Engine.SetPaused(true);
}

function resumeGame()
{
	Engine.GetGUIObjectByName("pauseButtonText").caption = translate("Pause");
	Engine.GetGUIObjectByName("pauseOverlay").hidden = true;
	Engine.SetPaused(false);
}

function togglePause()
{
	if (!Engine.GetGUIObjectByName("pauseButton").enabled)
		return;

	closeOpenDialogs();

	let pauseOverlay = Engine.GetGUIObjectByName("pauseOverlay");

	Engine.SetPaused(pauseOverlay.hidden);
	Engine.GetGUIObjectByName("pauseButtonText").caption = pauseOverlay.hidden ? translate("Resume") : translate("Pause");

	pauseOverlay.hidden = !pauseOverlay.hidden;
}

function openManual()
{
	closeOpenDialogs();
	pauseGame();

	Engine.PushGuiPage("page_manual.xml", {
		"page": "manual/intro",
		"title": translate("Manual"),
		"url": "http://trac.wildfiregames.com/wiki/0adManual",
		"callback": "resumeGame"
	});
}

function toggleDeveloperOverlay()
{
	// The developer overlay is disabled in ranked games
	if (Engine.HasXmppClient() && Engine.IsRankedGame())
		return;

	let devCommands = Engine.GetGUIObjectByName("devCommands");
	devCommands.hidden = !devCommands.hidden;

	let message = devCommands.hidden ?
		markForTranslation("The Developer Overlay was closed.") :
		markForTranslation("The Developer Overlay was opened.");

	Engine.PostNetworkCommand({
		"type": "aichat",
		"message": message,
		"translateMessage": true,
		"translateParameters": [],
		"parameters": {}
	});
}

function closeOpenDialogs()
{
	closeMenu();
	closeChat();
	closeDiplomacy();
	closeTrade();
}

function formatTributeTooltip(player, resource, amount)
{
	return sprintf(translate("Tribute %(resourceAmount)s %(resourceType)s to %(playerName)s. Shift-click to tribute %(greaterAmount)s."), {
		"resourceAmount": amount,
		"resourceType": getLocalizedResourceName(resource, "withinSentence"),
		"playerName": "[color=\"" + rgbToGuiColor(player.color) + "\"]" + player.name + "[/color]",
		"greaterAmount": (amount < 500 ? 500 : amount + 500)
	});
}
