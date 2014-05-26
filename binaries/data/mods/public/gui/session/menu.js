const PAUSE = translate("Pause");
const RESUME = translate("Resume");

/*
 * MENU POSITION CONSTANTS
*/

// Menu / panel border size
const MARGIN = 4;

// Includes the main menu button
const NUM_BUTTONS = 8;

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

// Trade menu: available resources and step for probability changes
const RESOURCES = ["food", "wood", "stone", "metal"];
const STEP = 5;

var isMenuOpen = false;
var menu;

var isDiplomacyOpen = false;
var isTradeOpen = false;

// Redefined every time someone makes a tribute (so we can save some data in a closure). Called in input.js handleInputBeforeGui.
var flushTributing = function() {};

// Ignore size defined in XML and set the actual menu size here
function initMenuPosition()
{
	menu = Engine.GetGUIObjectByName("menu");
	menu.size = INITIAL_MENU_POSITION;
}


// =============================================================================
// Overall Menu
// =============================================================================
//
// Slide menu
function updateMenuPosition(dt)
{
	if (isMenuOpen)
	{
		var maxOffset = END_MENU_POSITION - menu.size.bottom;
		if (maxOffset > 0)
		{
			var offset = Math.min(MENU_SPEED * dt, maxOffset);
			var size = menu.size;
			size.top += offset;
			size.bottom += offset;
			menu.size = size;
		}
	}
	else
	{
		var maxOffset = menu.size.top - MENU_TOP;
		if (maxOffset > 0)
		{
			var offset = Math.min(MENU_SPEED * dt, maxOffset);
			var size = menu.size;
			size.top -= offset;
			size.bottom -= offset;
			menu.size = size;
		}
	}
}

// Opens the menu by revealing the screen which contains the menu
function openMenu()
{
	isMenuOpen = true;
}

// Closes the menu and resets position
function closeMenu()
{
	isMenuOpen = false;
}

function toggleMenu()
{
	if (isMenuOpen == true)
		closeMenu();
	else
		openMenu();
}

// Menu buttons
// =============================================================================
function settingsMenuButton()
{
	closeMenu();
	closeOpenDialogs();
	openSettings();
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
	var btCaptions = [translate("Yes"), translate("No")];
	var btCode = [resignGame, resumeGame];
	messageBox(400, 200, translate("Are you sure you want to resign?"), translate("Confirmation"), 0, btCaptions, btCode);
}

function exitMenuButton()
{
	closeMenu();
	closeOpenDialogs();
	pauseGame();
	if (g_IsNetworked && g_IsController)
	{
		var btCode = [leaveGame, resumeGame];
		var message = translate("Are you sure you want to quit? Leaving will disconnect all other players.");
	}
	else if (g_IsNetworked && !g_GameEnded && !g_IsObserver)
	{
		var btCode = [networkReturnQuestion, resumeGame];
		var message = translate("Are you sure you want to quit?");
	}
	else
	{
		var btCode = [leaveGame, resumeGame];
		var message = translate("Are you sure you want to quit?");
	}
	messageBox(400, 200, message, translate("Confirmation"), 0, [translate("Yes"), translate("No")], btCode);
}

function networkReturnQuestion()
{
	var btCaptions = [translate("I will return"), translate("I resign")];
	var btCode = [leaveGame, resignGame];
	var btArgs = [true, false];
	messageBox(400, 200, translate("Do you want to resign or will you return soon?"), translate("Confirmation"), 0, btCaptions, btCode, btArgs);
}

function openDeleteDialog(selection)
{
	closeMenu();
	closeOpenDialogs();

	var deleteSelectedEntities = function (selectionArg)
	{
		Engine.PostNetworkCommand({"type": "delete-entities", "entities": selectionArg});
	};

	var btCaptions = [translate("Yes"), translate("No")];
	var btCode = [deleteSelectedEntities, resumeGame];
	var btArgs = [selection, null];

	messageBox(400, 200, translate("Destroy everything currently selected?"), translate("Delete"), 0, btCaptions, btCode, btArgs);
}

// Menu functions
// =============================================================================

function openSave()
{
	closeMenu();
	closeOpenDialogs();
	pauseGame();
	var savedGameData = getSavedGameData();
	Engine.PushGuiPage("page_savegame.xml", {"savedGameData":savedGameData, "callback":"resumeGame"});
}

function openSettings()
{
	pauseGame();
	Engine.PushGuiPage("page_options.xml", {"callback":"resumeGame"});
}

function openChat()
{
	Engine.GetGUIObjectByName("chatInput").focus(); // Grant focus to the input area
	Engine.GetGUIObjectByName("chatDialogPanel").hidden = false;
}

function closeChat()
{
	Engine.GetGUIObjectByName("chatInput").caption = ""; // Clear chat input
	Engine.GetGUIObjectByName("chatInput").blur(); // Remove focus
	Engine.GetGUIObjectByName("chatDialogPanel").hidden = true;
}

function toggleChatWindow(teamChat)
{
	var chatWindow = Engine.GetGUIObjectByName("chatDialogPanel");
	var chatInput = Engine.GetGUIObjectByName("chatInput");

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

	Engine.GetGUIObjectByName("toggleTeamChat").checked = teamChat;
	chatWindow.hidden = !chatWindow.hidden;
}

function setDiplomacy(data)
{
	Engine.PostNetworkCommand({"type": "diplomacy", "to": data.to, "player": data.player});
}

function tributeResource(data)
{
	Engine.PostNetworkCommand({"type": "tribute", "player": data.player, "amounts":  data.amounts});
}

function openDiplomacy()
{
	if (isTradeOpen)
		closeTrade();
	isDiplomacyOpen = true;

	var we = Engine.GetPlayerID();
	var players = getPlayerData(g_PlayerAssignments);

	// Get offset for one line
	var onesize = Engine.GetGUIObjectByName("diplomacyPlayer[0]").size;
	var rowsize = onesize.bottom - onesize.top;

	// We don't include gaia
	for (var i = 1; i < players.length; i++)
	{
		// Apply offset
		var row = Engine.GetGUIObjectByName("diplomacyPlayer["+(i-1)+"]");
		var size = row.size;
		size.top = rowsize*(i-1);
		size.bottom = rowsize*i;
		row.size = size;

		// Set background colour
		var playerColor = players[i].color.r+" "+players[i].color.g+" "+players[i].color.b;
		row.sprite = "colour: "+playerColor + " 32";

		Engine.GetGUIObjectByName("diplomacyPlayerName["+(i-1)+"]").caption = "[color=\"" + playerColor + "\"]" + players[i].name + "[/color]";
		Engine.GetGUIObjectByName("diplomacyPlayerCiv["+(i-1)+"]").caption = g_CivData[players[i].civ].Name;

		Engine.GetGUIObjectByName("diplomacyPlayerTeam["+(i-1)+"]").caption = (players[i].team < 0) ? translateWithContext("team", "None") : players[i].team+1;

		if (i != we)
			Engine.GetGUIObjectByName("diplomacyPlayerTheirs["+(i-1)+"]").caption = (players[i].isAlly[we] ? translate("Ally") : (players[i].isNeutral[we] ? translate("Neutral") : translate("Enemy")));

		// Don't display the options for ourself, or if we or the other player aren't active anymore
		if (i == we || players[we].state != "active" || players[i].state != "active")
		{
			// Hide the unused/unselectable options
			for each (var a in ["TributeFood", "TributeWood", "TributeStone", "TributeMetal", "Ally", "Neutral", "Enemy"])
				Engine.GetGUIObjectByName("diplomacyPlayer"+a+"["+(i-1)+"]").hidden = true;
			continue;
		}

		// Tribute
		for each (var resource in ["food", "wood", "stone", "metal"])
		{
			var button = Engine.GetGUIObjectByName("diplomacyPlayerTribute"+toTitleCase(resource)+"["+(i-1)+"]");
			button.onpress = (function(player, resource, button){
				// Implement something like how unit batch training works. Shift+click to send 500, shift+click+click to send 1000, etc.
				// Also see input.js (searching for "INPUT_MASSTRIBUTING" should get all the relevant parts).
				var multiplier = 1;
				return function() {
					var isBatchTrainPressed = Engine.HotkeyIsPressed("session.masstribute");
					if (isBatchTrainPressed)
					{
						inputState = INPUT_MASSTRIBUTING;
						multiplier += multiplier == 1 ? 4 : 5;
					}
					var amounts = {
						"food": (resource == "food" ? 100 : 0) * multiplier,
						"wood": (resource == "wood" ? 100 : 0) * multiplier,
						"stone": (resource == "stone" ? 100 : 0) * multiplier,
						"metal": (resource == "metal" ? 100 : 0) * multiplier,
					};
					button.tooltip = formatTributeTooltip(players[player], resource, amounts[resource]);
					// This is in a closure so that we have access to `player`, `amounts`, and `multiplier` without some
					// evil global variable hackery.
					flushTributing = function() {
						tributeResource({"player": player, "amounts": amounts});
						multiplier = 1;
						button.tooltip = formatTributeTooltip(players[player], resource, 100);
					};
					if (!isBatchTrainPressed)
						flushTributing();
				};
			})(i, resource, button);
			button.hidden = false;
			button.tooltip = formatTributeTooltip(players[i], resource, 100);
		}

		// Skip our own teams on teams locked
		if (players[we].teamsLocked && players[we].team != -1 && players[we].team == players[i].team)
			continue;

		// Diplomacy settings
		// Set up the buttons
		for each (var setting in ["ally", "neutral", "enemy"])
		{
			var button = Engine.GetGUIObjectByName("diplomacyPlayer"+toTitleCase(setting)+"["+(i-1)+"]");

			if (setting == "ally")
			{
				if (players[we].isAlly[i])
					button.caption = translate("x");
				else
					button.caption = "";
			}
			else if (setting == "neutral")
			{
				if (players[we].isNeutral[i])
					button.caption = translate("x");
				else
					button.caption = "";
			}
			else // "enemy"
			{
				if (players[we].isEnemy[i])
					button.caption = translate("x");
				else
					button.caption = "";
			}
			button.onpress = (function(e){ return function() { setDiplomacy(e) } })({"player": i, "to": setting});
			button.hidden = false;
		}
	}

	Engine.GetGUIObjectByName("diplomacyDialogPanel").hidden = false;
}

function closeDiplomacy()
{
	isDiplomacyOpen = false;
	Engine.GetGUIObjectByName("diplomacyDialogPanel").hidden = true;
}

function toggleDiplomacy()
{
	if (isDiplomacyOpen)
		closeDiplomacy();
	else
		openDiplomacy();
}

function openTrade()
{
	if (isDiplomacyOpen)
		closeDiplomacy();
	isTradeOpen = true;

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
	}

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
			}
		})(resource);

		buttonUp.onpress = (function(resource){
			return function() {
				proba[resource] += Math.min(STEP, proba[selec]);
				proba[selec]    -= Math.min(STEP, proba[selec]);
				Engine.PostNetworkCommand({"type": "set-trading-goods", "tradingGoods": proba});
				updateButtons();
			}
		})(resource);

		buttonDn.onpress = (function(resource){
			return function() {
				proba[selec]    += Math.min(STEP, proba[resource]);
				proba[resource] -= Math.min(STEP, proba[resource]);
				Engine.PostNetworkCommand({"type": "set-trading-goods", "tradingGoods": proba});
				updateButtons();
			}
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
				{
					caption = sprintf(translate("%(openingTradingString)s, %(garrisonedString)s, and %(inactiveString)s."), {
						openingTradingString: openingTradingString,
						garrisonedString: garrisonedString,
						inactiveString: inactiveString
					});
				}
				else
				{
					caption = sprintf(translate("%(openingTradingString)s, and %(garrisonedString)s."), {
						openingTradingString: openingTradingString,
						garrisonedString: garrisonedString
					});
				}
			}
			else
			{
				if (inactive > 0)
				{
					caption = sprintf(translate("%(openingTradingString)s, and %(inactiveString)s."), {
						openingTradingString: openingTradingString,
						inactiveString: inactiveString
					});
				}
				else
				{
					caption = sprintf(translate("%(openingTradingString)s."), {
						openingTradingString: openingTradingString,
					});
				}
			}
		}
		else
		{
			if (traderNumber.landTrader.garrisoned > 0)
			{
				var openingGarrisonedString = sprintf(translatePlural("There is %(numberGarrisoned)s land trader garrisoned on a trading merchant ship", "There are %(numberGarrisoned)s land traders garrisoned on a trading merchant ship", traderNumber.landTrader.garrisoned), { numberGarrisoned: traderNumber.landTrader.garrisoned });
				if (inactive > 0)
				{
					caption = sprintf(translate("%(openingGarrisonedString)s, and %(inactiveString)s."), {
						openingGarrisonedString: openingGarrisonedString,
						inactiveString: inactiveString
					});
				}
				else
				{
					caption = sprintf(translate("%(openingGarrisonedString)s."), {
						openingGarrisonedString: openingGarrisonedString
					});
				}
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
			{
				caption = sprintf(translate("%(openingTradingString)s, and %(inactiveString)s."), {
					openingTradingString: openingTradingString,
					inactiveString: inactiveString
				});
			}
			else
			{
				caption = sprintf(translate("%(openingTradingString)s."), {
					openingTradingString: openingTradingString,
				});
			}
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
	isTradeOpen = false;
	Engine.GetGUIObjectByName("tradeDialogPanel").hidden = true;
}

function toggleTrade()
{
	if (isTradeOpen)
		closeTrade();
	else
		openTrade();
}

function toggleGameSpeed()
{
	var gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
	gameSpeed.hidden = !gameSpeed.hidden;
}

/**
 * Pause the game in single player mode.
 */
function pauseGame()
{
	if (g_IsNetworked)
		return;

	Engine.GetGUIObjectByName("pauseButtonText").caption = RESUME;
	Engine.GetGUIObjectByName("pauseOverlay").hidden = false;
	Engine.SetPaused(true);
}

function resumeGame()
{
	Engine.GetGUIObjectByName("pauseButtonText").caption = PAUSE;
	Engine.GetGUIObjectByName("pauseOverlay").hidden = true;
	Engine.SetPaused(false);
}

function togglePause()
{
	closeMenu();
	closeOpenDialogs();

	var pauseOverlay = Engine.GetGUIObjectByName("pauseOverlay");

	if (pauseOverlay.hidden)
	{
		Engine.GetGUIObjectByName("pauseButtonText").caption = RESUME;
		Engine.SetPaused(true);
	}
	else
	{
		Engine.SetPaused(false);
		Engine.GetGUIObjectByName("pauseButtonText").caption = PAUSE;
	}

	pauseOverlay.hidden = !pauseOverlay.hidden;
}

function openManual()
{
	closeMenu();
	closeOpenDialogs();
	pauseGame();
	Engine.PushGuiPage("page_manual.xml", {"page": "manual/intro", "title":translate("Manual"), "url":"http://trac.wildfiregames.com/wiki/0adManual", "callback": "resumeGame"});
}

function toggleDeveloperOverlay()
{
	// The developer overlay is disabled in ranked games
	if (Engine.HasXmppClient() && Engine.IsRankedGame())
		return;

	var devCommands = Engine.GetGUIObjectByName("devCommands");
	if (devCommands.hidden)
		submitChatDirectly(translate("The Developer Overlay was opened."));
	else
		submitChatDirectly(translate("The Developer Overlay was closed."));
	// Toggle the overlay
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
	var playerColor = player.color.r + " " + player.color.g + " " + player.color.b;
	return sprintf(translate("Tribute %(resourceAmount)s %(resourceType)s to %(playerName)s. Shift-click to tribute %(greaterAmount)s."), {
		resourceAmount: amount,
		resourceType: getLocalizedResourceName(resource, "withinSentence"),
		playerName: "[color=\"" + playerColor + "\"]" + player.name + "[/color]",
		greaterAmount: (amount < 500 ? 500 : amount + 500)
	});
}
