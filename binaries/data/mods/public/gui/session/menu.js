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
	closeMenu();
	closeOpenDialogs();
	openOptions();
}

function chatMenuButton()
{
	closeMenu();
	closeOpenDialogs();
	openChat();
}

function diplomacyMenuButton()
{
	closeMenu();
	closeOpenDialogs();
	openDiplomacy();
}

function pauseMenuButton()
{
	togglePause();
}

function resignMenuButton()
{
	closeMenu();
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
	closeMenu();
	closeOpenDialogs();
	pauseGame();

	let messageTypes = {
		"host": {
			"caption": translate("Are you sure you want to quit? Leaving will disconnect all other players."),
			"buttons": [resumeGame, leaveGame]
		},
		"client": {
			"caption": translate("Are you sure you want to quit?"),
			"buttons": [resumeGame, networkReturnQuestion]
		},
		"singleplayer": {
			"caption": translate("Are you sure you want to quit?"),
			"buttons": [resumeGame, leaveGame]
		}
	};

	let messageType = g_IsNetworked && g_IsController ? "host" :
		(g_IsNetworked && !g_GameEnded && !g_IsObserver ? "client" : "singleplayer");

	messageBox(
		400, 200,
		messageTypes[messageType].caption,
		translate("Confirmation"),
		0,
		[translate("No"), translate("Yes")],
		messageTypes[messageType].buttons
	);
}

function networkReturnQuestion()
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
	closeMenu();
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
	closeMenu();
	closeOpenDialogs();
	pauseGame();
	Engine.PushGuiPage("page_savegame.xml", { "savedGameData": getSavedGameData(), "callback": "resumeGame" });
}

function openOptions()
{
	pauseGame();
	Engine.PushGuiPage("page_options.xml", { "callback": "resumeGame" });
}

function openChat()
{
	if (g_Disconnected)
		return;

	updateTeamCheckbox(false);

	Engine.GetGUIObjectByName("chatInput").focus(); // Grant focus to the input area
	Engine.GetGUIObjectByName("chatDialogPanel").hidden = false;
}

function closeChat()
{
	Engine.GetGUIObjectByName("chatInput").caption = ""; // Clear chat input
	Engine.GetGUIObjectByName("chatInput").blur(); // Remove focus
	Engine.GetGUIObjectByName("chatDialogPanel").hidden = true;
}

function updateTeamCheckbox(check)
{
	Engine.GetGUIObjectByName("toggleTeamChatLabel").hidden = g_IsObserver;
	let toggleTeamChat = Engine.GetGUIObjectByName("toggleTeamChat");
	toggleTeamChat.hidden = g_IsObserver;
	toggleTeamChat.checked = !g_IsObserver && check;
}

function toggleChatWindow(teamChat)
{
	if (g_Disconnected)
		return;

	let chatWindow = Engine.GetGUIObjectByName("chatDialogPanel");
	let chatInput = Engine.GetGUIObjectByName("chatInput");

	if (chatWindow.hidden)
		chatInput.focus(); // Grant focus to the input area
	else
	{
		if (chatInput.caption.length)
		{
			submitChatInput();
			return;
		}
		chatInput.caption = ""; // Clear chat input
	}

	updateTeamCheckbox(teamChat);
	chatWindow.hidden = !chatWindow.hidden;
}

function setDiplomacy(data)
{
	Engine.PostNetworkCommand({ "type": "diplomacy", "to": data.to, "player": data.player });
}

function tributeResource(data)
{
	Engine.PostNetworkCommand({ "type": "tribute", "player": data.player, "amounts":  data.amounts });
}

function openDiplomacy()
{
	if (g_IsTradeOpen)
		closeTrade();
	g_IsDiplomacyOpen = true;

	let we = Engine.GetPlayerID();

	// Get offset for one line
	let onesize = Engine.GetGUIObjectByName("diplomacyPlayer[0]").size;
	let rowsize = onesize.bottom - onesize.top;

	// We don't include gaia
	for (let i = 1; i < g_Players.length; ++i)
	{
		// Apply offset
		let row = Engine.GetGUIObjectByName("diplomacyPlayer["+(i-1)+"]");
		let size = row.size;
		size.top = rowsize*(i-1);
		size.bottom = rowsize*i;
		row.size = size;

		// Set background color
		let playerColor = rgbToGuiColor(g_Players[i].color);
		row.sprite = "color: "+playerColor + " 32";

		Engine.GetGUIObjectByName("diplomacyPlayerName["+(i-1)+"]").caption = "[color=\"" + playerColor + "\"]" + g_Players[i].name + "[/color]";
		Engine.GetGUIObjectByName("diplomacyPlayerCiv["+(i-1)+"]").caption = g_CivData[g_Players[i].civ].Name;

		Engine.GetGUIObjectByName("diplomacyPlayerTeam["+(i-1)+"]").caption = (g_Players[i].team < 0) ? translateWithContext("team", "None") : g_Players[i].team+1;

		if (i != we)
			Engine.GetGUIObjectByName("diplomacyPlayerTheirs["+(i-1)+"]").caption = (g_Players[i].isAlly[we] ? translate("Ally") : (g_Players[i].isNeutral[we] ? translate("Neutral") : translate("Enemy")));

		// Don't display the options for ourself, or if we or the other player aren't active anymore
		if (i == we || g_Players[we].state != "active" || g_Players[i].state != "active")
		{
			// Hide the unused/unselectable options
			for (let a of ["TributeFood", "TributeWood", "TributeStone", "TributeMetal", "Ally", "Neutral", "Enemy"])
				Engine.GetGUIObjectByName("diplomacyPlayer"+a+"["+(i-1)+"]").hidden = true;
			Engine.GetGUIObjectByName("diplomacyAttackRequest["+(i-1)+"]").hidden = true;
			continue;
		}

		// Tribute
		for (let resource of RESOURCES)
		{
			let button = Engine.GetGUIObjectByName("diplomacyPlayerTribute"+resource[0].toUpperCase()+resource.substring(1)+"["+(i-1)+"]");
			button.onpress = (function(player, resource, button){
				// Implement something like how unit batch training works. Shift+click to send 500, shift+click+click to send 1000, etc.
				// Also see input.js (searching for "INPUT_MASSTRIBUTING" should get all the relevant parts).
				let multiplier = 1;
				return function() {
					let isBatchTrainPressed = Engine.HotkeyIsPressed("session.masstribute");
					if (isBatchTrainPressed)
					{
						inputState = INPUT_MASSTRIBUTING;
						multiplier += multiplier == 1 ? 4 : 5;
					}
					let amounts = {
						"food": (resource == "food" ? 100 : 0) * multiplier,
						"wood": (resource == "wood" ? 100 : 0) * multiplier,
						"stone": (resource == "stone" ? 100 : 0) * multiplier,
						"metal": (resource == "metal" ? 100 : 0) * multiplier
					};
					button.tooltip = formatTributeTooltip(g_Players[player], resource, amounts[resource]);
					// This is in a closure so that we have access to `player`, `amounts`, and `multiplier` without some
					// evil global variable hackery.
					g_FlushTributing = function() {
						tributeResource({ "player": player, "amounts": amounts });
						multiplier = 1;
						button.tooltip = formatTributeTooltip(g_Players[player], resource, 100);
					};
					if (!isBatchTrainPressed)
						g_FlushTributing();
				};
			})(i, resource, button);
			button.hidden = false;
			button.tooltip = formatTributeTooltip(g_Players[i], resource, 100);
		}

		// Attack Request
		let simState = GetSimState();
		let button = Engine.GetGUIObjectByName("diplomacyAttackRequest["+(i-1)+"]");
		button.hidden = simState.ceasefireActive || !(g_Players[i].isEnemy[we]);
		button.tooltip = translate("Request your allies to attack this enemy");
		button.onpress = (function(i, we){ return function() {
			Engine.PostNetworkCommand({ "type": "attack-request", "source": we, "target": i });
		}; })(i, we);

		// Skip our own teams on teams locked
		if (g_Players[we].teamsLocked && g_Players[we].team != -1 && g_Players[we].team == g_Players[i].team)
			continue;

		// Diplomacy settings
		// Set up the buttons
		for (let setting of ["Ally", "Neutral", "Enemy"])
		{
			let button = Engine.GetGUIObjectByName("diplomacyPlayer"+setting+"["+(i-1)+"]");

			button.caption = g_Players[we]["is"+setting][i] ? translate("x") : "";
			button.onpress = (function(e){ return function() { setDiplomacy(e); }; })({ "player": i, "to": setting.toLowerCase() });
			button.hidden = simState.ceasefireActive;
		}
	}

	Engine.GetGUIObjectByName("diplomacyDialogPanel").hidden = false;
}

function closeDiplomacy()
{
	g_IsDiplomacyOpen = false;
	Engine.GetGUIObjectByName("diplomacyDialogPanel").hidden = true;
}

function toggleDiplomacy()
{
	if (g_IsDiplomacyOpen)
		closeDiplomacy();
	else
		openDiplomacy();
}

function openTrade()
{
	if (g_IsDiplomacyOpen)
		closeDiplomacy();
	g_IsTradeOpen = true;

	var updateButtons = function()
	{
		for (var res in button)
		{
			button[res].label.caption = proba[res] + "%";
			if (res == selec)
			{
				button[res].sel.hidden = false;
				button[res].up.hidden = true;
				button[res].dn.hidden = true;
			}
			else
			{
				button[res].sel.hidden = true;
				button[res].up.hidden = (proba[res] == 100 || proba[selec] == 0);
				button[res].dn.hidden = (proba[res] == 0 || proba[selec] == 100);
			}
		}
	};

	var proba = Engine.GuiInterfaceCall("GetTradingGoods");
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

		buttonUp.onpress = (function(resource){
			return function() {
				proba[resource] += Math.min(STEP, proba[selec]);
				proba[selec]    -= Math.min(STEP, proba[selec]);
				Engine.PostNetworkCommand({"type": "set-trading-goods", "tradingGoods": proba});
				updateButtons();
			};
		})(resource);

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

	var traderNumber = Engine.GuiInterfaceCall("GetTraderNumber");
	var caption = "";
	if (traderNumber.landTrader.total == 0)
		caption = translate("There are no land traders.");
	else
	{
		var inactive = traderNumber.landTrader.total - traderNumber.landTrader.trading - traderNumber.landTrader.garrisoned;
		var inactiveString = "";
		if (inactive > 0)
			inactiveString = "[color=\"orange\"]" + sprintf(translatePlural("%(numberOfLandTraders)s inactive", "%(numberOfLandTraders)s inactive", inactive), { numberOfLandTraders: inactive }) + "[/color]";

		if (traderNumber.landTrader.trading > 0)
		{
			var openingTradingString = sprintf(translatePlural("There is %(numberTrading)s land trader trading", "There are %(numberTrading)s land traders trading", traderNumber.landTrader.trading), { numberTrading: traderNumber.landTrader.trading });
			if (traderNumber.landTrader.garrisoned > 0)
			{
				var garrisonedString = sprintf(translatePlural("%(numberGarrisoned)s garrisoned on a trading merchant ship", "%(numberGarrisoned)s garrisoned on a trading merchant ship", traderNumber.landTrader.garrisoned), { numberGarrisoned: traderNumber.landTrader.garrisoned });
				if (inactive > 0)
					caption = sprintf(translate("%(openingTradingString)s, %(garrisonedString)s, and %(inactiveString)s."), {
						openingTradingString: openingTradingString,
						garrisonedString: garrisonedString,
						inactiveString: inactiveString
					});
				else
					caption = sprintf(translate("%(openingTradingString)s, and %(garrisonedString)s."), {
						openingTradingString: openingTradingString,
						garrisonedString: garrisonedString
					});
			}
			else
			{
				if (inactive > 0)
					caption = sprintf(translate("%(openingTradingString)s, and %(inactiveString)s."), {
						openingTradingString: openingTradingString,
						inactiveString: inactiveString
					});
				else
					caption = sprintf(translate("%(openingTradingString)s."), {
						openingTradingString: openingTradingString,
					});
			}
		}
		else
		{
			if (traderNumber.landTrader.garrisoned > 0)
			{
				var openingGarrisonedString = sprintf(translatePlural("There is %(numberGarrisoned)s land trader garrisoned on a trading merchant ship", "There are %(numberGarrisoned)s land traders garrisoned on a trading merchant ship", traderNumber.landTrader.garrisoned), { numberGarrisoned: traderNumber.landTrader.garrisoned });
				if (inactive > 0)
					caption = sprintf(translate("%(openingGarrisonedString)s, and %(inactiveString)s."), {
						openingGarrisonedString: openingGarrisonedString,
						inactiveString: inactiveString
					});
				else
					caption = sprintf(translate("%(openingGarrisonedString)s."), {
						openingGarrisonedString: openingGarrisonedString
					});
			}
			else
			{
				if (inactive > 0)
				{
					inactiveString = "[color=\"orange\"]" + sprintf(translatePlural("%(numberOfLandTraders)s land trader inactive", "%(numberOfLandTraders)s land traders inactive", inactive), { numberOfLandTraders: inactive }) + "[/color]";
					caption = sprintf(translatePlural("There is %(inactiveString)s.", "There are %(inactiveString)s.", inactive), {
						inactiveString: inactiveString
					});
				}
				// The “else” here is already handled by “if (traderNumber.landTrader.total == 0)” above.
			}
		}
	}
	Engine.GetGUIObjectByName("landTraders").caption = caption;

	caption = "";
	if (traderNumber.shipTrader.total == 0)
		caption = translate("There are no merchant ships.");
	else
	{
		var inactive = traderNumber.shipTrader.total - traderNumber.shipTrader.trading;
		var inactiveString = "";
		if (inactive > 0)
			inactiveString = "[color=\"orange\"]" + sprintf(translatePlural("%(numberOfShipTraders)s inactive", "%(numberOfShipTraders)s inactive", inactive), { numberOfShipTraders: inactive }) + "[/color]";

		if (traderNumber.shipTrader.trading > 0)
		{
			var openingTradingString = sprintf(translatePlural("There is %(numberTrading)s merchant ship trading", "There are %(numberTrading)s merchant ships trading", traderNumber.shipTrader.trading), { numberTrading: traderNumber.shipTrader.trading });
			if (inactive > 0)
				caption = sprintf(translate("%(openingTradingString)s, and %(inactiveString)s."), {
					openingTradingString: openingTradingString,
					inactiveString: inactiveString
				});
			else
				caption = sprintf(translate("%(openingTradingString)s."), {
					openingTradingString: openingTradingString,
				});
		}
		else
		{
			if (inactive > 0)
			{
				inactiveString = "[color=\"orange\"]" + sprintf(translatePlural("%(numberOfShipTraders)s merchant ship inactive", "%(numberOfShipTraders)s merchant ships inactive", inactive), { numberOfShipTraders: inactive }) + "[/color]";
				caption = sprintf(translatePlural("There is %(inactiveString)s.", "There are %(inactiveString)s.", inactive), {
					inactiveString: inactiveString
				});
			}
			// The “else” here is already handled by “if (traderNumber.shipTrader.total == 0)” above.
		}
	}
	Engine.GetGUIObjectByName("shipTraders").caption = caption;

	Engine.GetGUIObjectByName("tradeDialogPanel").hidden = false;
}

function closeTrade()
{
	g_IsTradeOpen = false;
	Engine.GetGUIObjectByName("tradeDialogPanel").hidden = true;
}

function toggleTrade()
{
	if (g_IsTradeOpen)
		closeTrade();
	else
		openTrade();
}

function toggleGameSpeed()
{
	let gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
	gameSpeed.hidden = !gameSpeed.hidden;
}

function openGameSummary()
{
	closeMenu();
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
	closeMenu();
	closeOpenDialogs();
	pauseGame();

	// TODO add info about researched techs and unlocked entities
	Engine.PushGuiPage("page_structree.xml", {
		"civ" : g_Players[Engine.GetPlayerID()].civ,
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

	closeMenu();
	closeOpenDialogs();

	let pauseOverlay = Engine.GetGUIObjectByName("pauseOverlay");

	Engine.SetPaused(pauseOverlay.hidden);
	Engine.GetGUIObjectByName("pauseButtonText").caption = pauseOverlay.hidden ? translate("Resume") : translate("Pause");

	pauseOverlay.hidden = !pauseOverlay.hidden;
}

function openManual()
{
	closeMenu();
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
	if (devCommands.hidden)
		submitChatDirectly(translate("The Developer Overlay was opened."));
	else
		submitChatDirectly(translate("The Developer Overlay was closed."));

	devCommands.hidden = !devCommands.hidden;
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
