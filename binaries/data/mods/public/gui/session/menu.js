// Menu / panel border size
var MARGIN = 4;

// Includes the main menu button
const NUM_BUTTONS = 10;

// Regular menu buttons
var BUTTON_HEIGHT = 32;

// The position where the bottom of the menu will end up (currently 228)
const END_MENU_POSITION = (BUTTON_HEIGHT * NUM_BUTTONS) + MARGIN;

// Menu starting position: bottom
const MENU_BOTTOM = 0;

// Menu starting position: top
const MENU_TOP = MENU_BOTTOM - END_MENU_POSITION;

// Number of pixels per millisecond to move
var MENU_SPEED = 1.2;

// Trade menu: step for probability changes
var STEP = 5;

// Shown in the trade dialog.
var g_IdleTraderTextColor = "orange";

/**
 * Store civilization code and page (structree or history) opened in civilization info.
 */
var g_CivInfo = {
	"civ": "",
	"page": "page_structree.xml"
};

/**
 * The barter constants should match with the simulation
 * Quantity of goods to sell per click.
 */
const g_BarterResourceSellQuantity = 100;

/**
 * Multiplier to be applied when holding the massbarter hotkey.
 */
const g_BarterMultiplier = 5;

/**
 * Barter actions, as mapped to the names of GUI Buttons.
 */
const g_BarterActions = ["Buy", "Sell"];

/**
 * Currently selected resource type to sell in the barter GUI.
 */
var g_BarterSell;

var g_IsMenuOpen = false;

var g_IsDiplomacyOpen = false;
var g_IsTradeOpen = false;
var g_IsObjectivesOpen = false;

/**
 * Used to disable a specific bribe button for the time we are waiting for the result of the bribe after it was clicked.
 * It contains an array per viewedPlayer. This array is a list of the players that were bribed.
 */
var g_BribeButtonsWaiting = {};

/**
 * Remember last viewed summary panel and charts.
 */
var g_SummarySelectedData;

// Redefined every time someone makes a tribute (so we can save some data in a closure). Called in input.js handleInputBeforeGui.
var g_FlushTributing = function() {};

function initMenu()
{
	Engine.GetGUIObjectByName("menu").size = "100%-164 " + MENU_TOP + " 100% " + MENU_BOTTOM;

	// TODO: Atlas should pass g_GameAttributes.settings
	for (let button of ["menuExitButton", "summaryButton", "objectivesButton", "diplomacyButton"])
		Engine.GetGUIObjectByName(button).enabled = !Engine.IsAtlasRunning();
}

function updateMenuPosition(dt)
{
	let menu = Engine.GetGUIObjectByName("menu");

	let maxOffset = g_IsMenuOpen ?
		END_MENU_POSITION - menu.size.bottom :
		menu.size.top - MENU_TOP;

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

function lobbyDialogButton()
{
	if (!Engine.HasXmppClient())
		return;

	closeOpenDialogs();
	Engine.PushGuiPage("page_lobby.xml", { "dialog": true });
}

function chatMenuButton()
{
	closeOpenDialogs();
	openChat();
}

function resignMenuButton()
{
	closeOpenDialogs();
	pauseGame();

	messageBox(
		400, 200,
		translate("Are you sure you want to resign?"),
		translate("Confirmation"),
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
		[translate("I will return"), translate("I resign")],
		[leaveGame, resignGame],
		[true, false]
	);
}

function openDeleteDialog(selection)
{
	closeOpenDialogs();

	let deleteSelectedEntities = function(selectionArg)
	{
		Engine.PostNetworkCommand({
			"type": "delete-entities",
			"entities": selectionArg
		});
	};

	messageBox(
		400, 200,
		translate("Destroy everything currently selected?"),
		translate("Delete"),
		[translate("No"), translate("Yes")],
		[resumeGame, deleteSelectedEntities],
		[null, selection]
	);
}

function openSave()
{
	closeOpenDialogs();
	pauseGame();

	Engine.PushGuiPage(
		"page_loadgame.xml",
		{ "savedGameData": getSavedGameData() },
		resumeGame);
}

function openOptions()
{
	closeOpenDialogs();
	pauseGame();

	Engine.PushGuiPage(
		"page_options.xml",
		{},
		callbackFunctionNames => {
			for (let functionName of callbackFunctionNames)
				if (global[functionName])
					global[functionName]();

			resumeGame();
		});
}

function openChat(command = "")
{
	if (g_Disconnected)
		return;

	closeOpenDialogs();

	let chatAddressee = Engine.GetGUIObjectByName("chatAddressee");
	chatAddressee.selected = chatAddressee.list_data.indexOf(command);

	Engine.GetGUIObjectByName("chatInput").focus();
	Engine.GetGUIObjectByName("chatDialogPanel").hidden = false;

	updateChatHistory();
}

function closeChat()
{
	Engine.GetGUIObjectByName("chatInput").caption = "";
	Engine.GetGUIObjectByName("chatInput").blur(); // Remove focus
	Engine.GetGUIObjectByName("chatDialogPanel").hidden = true;
}

function resizeDiplomacyDialog()
{
	let dialog = Engine.GetGUIObjectByName("diplomacyDialogPanel");
	let size = dialog.size;

	let resTribCodesLength = g_ResourceData.GetTributableCodes().length;
	if (resTribCodesLength)
	{
		let tribSize = Engine.GetGUIObjectByName("diplomacyPlayer[0]_tribute[0]").size;
		let widthOffset = resTribCodesLength * (tribSize.right - tribSize.left) / 2;
		size.left -= widthOffset;
		size.right += widthOffset;
	}
	else
		Engine.GetGUIObjectByName("diplomacyHeaderTribute").hidden = true;

	let firstRow = Engine.GetGUIObjectByName("diplomacyPlayer[0]").size;
	let heightOffset = (g_Players.length - 1) * (firstRow.bottom - firstRow.top) / 2;
	size.top -= heightOffset;
	size.bottom += heightOffset;

	dialog.size = size;
}

function initChatWindow()
{
	let filters = prepareForDropdown(g_ChatHistoryFilters);
	let chatHistoryFilter = Engine.GetGUIObjectByName("chatHistoryFilter");
	chatHistoryFilter.list = filters.text;
	chatHistoryFilter.list_data = filters.key;
	chatHistoryFilter.selected = 0;

	Engine.GetGUIObjectByName("extendedChat").checked =
		Engine.ConfigDB_GetValue("user", "chat.session.extended") == "true";

	resizeChatWindow();
}

function resizeChatWindow()
{
	// Hide/show the panel
	let chatHistoryPage = Engine.GetGUIObjectByName("chatHistoryPage");
	let extended = Engine.GetGUIObjectByName("extendedChat").checked;
	chatHistoryPage.hidden = !extended;

	// Resize the window
	let chatDialogPanel = Engine.GetGUIObjectByName("chatDialogPanel");
	if (extended)
	{
		chatDialogPanel.size = Engine.GetGUIObjectByName("chatDialogPanelLarge").size;
		// Adjust the width so that the chat history is in the golden ratio
		let chatHistory = Engine.GetGUIObjectByName("chatHistory");
		let height = chatHistory.getComputedSize().bottom - chatHistory.getComputedSize().top;
		let width = (1 + Math.sqrt(5)) / 2 * height;
		let size = chatDialogPanel.size;
		size.left = -width / 2 - chatHistory.size.left;
		size.right = width / 2 + chatHistory.size.left;
		chatDialogPanel.size = size;
	}
	else
		chatDialogPanel.size = Engine.GetGUIObjectByName("chatDialogPanelSmall").size;
}

function updateChatHistory()
{
	if (Engine.GetGUIObjectByName("chatDialogPanel").hidden ||
	    !Engine.GetGUIObjectByName("extendedChat").checked)
		return;

	let chatHistoryFilter = Engine.GetGUIObjectByName("chatHistoryFilter");
	let selected = chatHistoryFilter.list_data[chatHistoryFilter.selected];

	Engine.GetGUIObjectByName("chatHistory").caption =
		g_ChatHistory.filter(msg => msg.filter[selected]).map(msg =>
			Engine.ConfigDB_GetValue("user", "chat.timestamp") == "true" ?
				sprintf(translate("%(time)s %(message)s"), {
					"time": msg.timePrefix,
					"message": msg.txt
				}) :
				msg.txt
		).join("\n");
}

function onToggleChatWindowExtended()
{
	Engine.ConfigDB_CreateAndWriteValueToFile("user", "chat.session.extended", String(Engine.GetGUIObjectByName("extendedChat").checked), "config/user.cfg");

	resizeChatWindow();

	Engine.GetGUIObjectByName("chatInput").focus();
}

function openDiplomacy()
{
	closeOpenDialogs();

	if (g_ViewedPlayer < 1)
		return;

	g_IsDiplomacyOpen = true;

	updateDiplomacy(true);

	Engine.GetGUIObjectByName("diplomacyDialogPanel").hidden = false;
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

function updateDiplomacy(opening = false)
{
	if (g_ViewedPlayer < 1 || !g_IsDiplomacyOpen)
		return;

	let simState = GetSimState();
	let isCeasefireActive = simState.ceasefireActive;
	let hasSharedLos = GetSimState().players[g_ViewedPlayer].hasSharedLos;

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
		// Tribute buttons do not need to be updated onTick, and should not because of massTributing
		if (opening)
			diplomacyFormatTributeButtons(i, myself || playerInactive);
		diplomacyFormatAttackRequestButton(i, myself || playerInactive || isCeasefireActive || !hasAllies || !g_Players[i].isEnemy[g_ViewedPlayer]);
		diplomacyFormatSpyRequestButton(i, myself || playerInactive || g_Players[i].isMutualAlly[g_ViewedPlayer] && hasSharedLos);
	}

	let diplomacyCeasefireCounter = Engine.GetGUIObjectByName("diplomacyCeasefireCounter");
	diplomacyCeasefireCounter.caption = sprintf(
		translateWithContext("ceasefire", "Remaining ceasefire time: %(time)s."),
		{ "time": timeToString(simState.ceasefireTimeRemaining) }
	);
	diplomacyCeasefireCounter.hidden = !isCeasefireActive;
}

function diplomacySetupTexts(i, rowsize)
{
	// Apply offset
	let row = Engine.GetGUIObjectByName("diplomacyPlayer[" + (i - 1) + "]");
	let size = row.size;
	size.top = rowsize * (i - 1);
	size.bottom = rowsize * i;
	row.size = size;
	row.hidden = false;

	row.sprite = "color:" + rgbToGuiColor(g_DisplayedPlayerColors[i], 32);

	setOutcomeIcon(g_Players[i].state, "diplomacyPlayerOutcome[" + (i - 1) + "]");

	let diplomacyPlayerName = Engine.GetGUIObjectByName("diplomacyPlayerName[" + (i - 1) + "]");
	diplomacyPlayerName.caption = colorizePlayernameByID(i);
	diplomacyPlayerName.tooltip = translateAISettings(g_GameAttributes.settings.PlayerData[i]);

	Engine.GetGUIObjectByName("diplomacyPlayerCiv[" + (i - 1) + "]").caption = g_CivData[g_Players[i].civ].Name;

	Engine.GetGUIObjectByName("diplomacyPlayerTeam[" + (i - 1) + "]").caption =
		g_Players[i].team < 0 ? translateWithContext("team", "None") : g_Players[i].team + 1;

	Engine.GetGUIObjectByName("diplomacyPlayerTheirs[" + (i - 1) + "]").caption =
		i == g_ViewedPlayer ? "" :
			g_Players[i].isAlly[g_ViewedPlayer] ?
				translate("Ally") :
				g_Players[i].isNeutral[g_ViewedPlayer] ? translate("Neutral") : translate("Enemy");
}

function diplomacyFormatStanceButtons(i, hidden)
{
	for (let stance of ["Ally", "Neutral", "Enemy"])
	{
		let button = Engine.GetGUIObjectByName("diplomacyPlayer" + stance + "[" + (i - 1) + "]");
		button.hidden = hidden;
		if (hidden)
			continue;

		let isCurrentStance = g_Players[g_ViewedPlayer]["is" + stance][i];
		button.caption = isCurrentStance ? translate("x") : "";
		button.enabled = controlsPlayer(g_ViewedPlayer) && !isCurrentStance;

		button.onPress = (function(player, stance) { return function() {
			Engine.PostNetworkCommand({
				"type": "diplomacy",
				"player": i,
				"to": stance.toLowerCase()
			});
		}; })(i, stance);
	}
}

function diplomacyFormatTributeButtons(i, hidden)
{
	let resTribCodes = g_ResourceData.GetTributableCodes();
	let r = 0;
	for (let resCode of resTribCodes)
	{
		let button = Engine.GetGUIObjectByName("diplomacyPlayer[" + (i - 1) + "]_tribute[" + r + "]");
		if (!button)
		{
			warn("Current GUI limits prevent displaying more than " + r + " tribute buttons!");
			break;
		}

		Engine.GetGUIObjectByName("diplomacyPlayer[" + (i - 1) + "]_tribute[" + r + "]_image").sprite = "stretched:session/icons/resources/" + resCode + ".png";
		button.hidden = hidden;
		setPanelObjectPosition(button, r, r + 1, 0);
		++r;
		if (hidden)
			continue;

		button.enabled = controlsPlayer(g_ViewedPlayer);
		button.tooltip = formatTributeTooltip(i, resCode, 100);
		button.onPress = (function(i, resCode, button) {
			// Shift+click to send 500, shift+click+click to send 1000, etc.
			// See INPUT_MASSTRIBUTING in input.js
			let multiplier = 1;
			return function() {
				let isMassTributePressed = Engine.HotkeyIsPressed("session.masstribute");
				if (isMassTributePressed)
				{
					inputState = INPUT_MASSTRIBUTING;
					multiplier += multiplier == 1 ? 4 : 5;
				}

				let amounts = {};
				for (let res of resTribCodes)
					amounts[res] = 0;
				amounts[resCode] = 100 * multiplier;

				button.tooltip = formatTributeTooltip(i, resCode, amounts[resCode]);

				// This is in a closure so that we have access to `player`, `amounts`, and `multiplier` without some
				// evil global variable hackery.
				g_FlushTributing = function() {
					Engine.PostNetworkCommand({ "type": "tribute", "player": i, "amounts": amounts });
					multiplier = 1;
					button.tooltip = formatTributeTooltip(i, resCode, 100);
				};

				if (!isMassTributePressed)
					g_FlushTributing();
			};
		})(i, resCode, button);
	}
}

function diplomacyFormatAttackRequestButton(i, hidden)
{
	let button = Engine.GetGUIObjectByName("diplomacyAttackRequest[" + (i - 1) + "]");
	button.hidden = hidden;
	if (hidden)
		return;

	button.enabled = controlsPlayer(g_ViewedPlayer);
	button.tooltip = translate("Request your allies to attack this enemy");
	button.onPress = (function(i) { return function() {
		Engine.PostNetworkCommand({ "type": "attack-request", "source": g_ViewedPlayer, "player": i });
	}; })(i);
}

function diplomacyFormatSpyRequestButton(i, hidden)
{
	let button = Engine.GetGUIObjectByName("diplomacySpyRequest[" + (i - 1) + "]");
	let template = GetTemplateData("special/spy");
	button.hidden = hidden || !template || !!GetSimState().players[g_ViewedPlayer].disabledTemplates["special/spy"];
	if (button.hidden)
		return;

	button.enabled = controlsPlayer(g_ViewedPlayer) &&
	                 !(g_BribeButtonsWaiting[g_ViewedPlayer] && g_BribeButtonsWaiting[g_ViewedPlayer].indexOf(i) != -1);
	let modifier = "";
	let tooltips = [translate("Bribe a random unit from this player and share its vision during a limited period.")];
	if (!button.enabled)
		modifier = "color:0 0 0 127:grayscale:";
	else
	{
		if (template.requiredTechnology)
		{
			let technologyEnabled = Engine.GuiInterfaceCall("IsTechnologyResearched", {
				"tech": template.requiredTechnology,
				"player": g_ViewedPlayer
			});
			if (!technologyEnabled)
			{
				modifier = "color:0 0 0 127:grayscale:";
				button.enabled = false;
				tooltips.push(getRequiredTechnologyTooltip(technologyEnabled, template.requiredTechnology, GetSimState().players[g_ViewedPlayer].civ));
			}
		}

		if (template.cost)
		{
			let modifiedTemplate = clone(template);
			for (let res in template.cost)
				modifiedTemplate.cost[res] = Math.floor(GetSimState().players[i].spyCostMultiplier * template.cost[res]);
			tooltips.push(getEntityCostTooltip(modifiedTemplate));
			let neededResources = Engine.GuiInterfaceCall("GetNeededResources", {
				"cost": modifiedTemplate.cost,
				"player": g_ViewedPlayer
			});
			let costRatio = Engine.GetTemplate("special/spy").VisionSharing.FailureCostRatio;
			if (costRatio > 0)
			{
				tooltips.push(translate("A failed bribe will cost you:"));
				for (let res in modifiedTemplate.cost)
					modifiedTemplate.cost[res] = Math.floor(costRatio * modifiedTemplate.cost[res]);
				tooltips.push(getEntityCostTooltip(modifiedTemplate));
			}

			if (neededResources)
			{
				if (button.enabled)
					modifier = resourcesToAlphaMask(neededResources) + ":";
				button.enabled = false;
				tooltips.push(getNeededResourcesTooltip(neededResources));
			}
		}
	}
	let icon = Engine.GetGUIObjectByName("diplomacySpyRequestImage[" + (i - 1) + "]");
	icon.sprite = modifier + "stretched:session/icons/bribes.png";
	button.tooltip = tooltips.filter(tip => tip).join("\n");
	button.onPress = (function(i, button) { return function() {
		Engine.PostNetworkCommand({ "type": "spy-request", "source": g_ViewedPlayer, "player": i });
		if (!g_BribeButtonsWaiting[g_ViewedPlayer])
			g_BribeButtonsWaiting[g_ViewedPlayer] = [];
		// Don't push i twice
		if (g_BribeButtonsWaiting[g_ViewedPlayer].indexOf(i) == -1)
			g_BribeButtonsWaiting[g_ViewedPlayer].push(i);
		diplomacyFormatSpyRequestButton(i, false);
	}; })(i, button);
}

function resizeTradeDialog()
{
	let dialog = Engine.GetGUIObjectByName("tradeDialogPanel");
	let size = dialog.size;
	let width = size.right - size.left;

	let resTradCodesLength = g_ResourceData.GetTradableCodes().length;
	Engine.GetGUIObjectByName("tradeDialogPanelTrade").hidden = !resTradCodesLength;

	let resBarterCodesLength = g_ResourceData.GetBarterableCodes().length;
	Engine.GetGUIObjectByName("tradeDialogPanelBarter").hidden = !resBarterCodesLength;

	let tradeSize = Engine.GetGUIObjectByName("tradeResource[0]").size;
	let length = Math.max(resTradCodesLength, resBarterCodesLength);
	width += length * (tradeSize.right - tradeSize.left);

	size.left = -width / 2;
	size.right = width / 2;
	dialog.size = size;
}

function openTrade()
{
	closeOpenDialogs();

	if (g_ViewedPlayer < 1)
		return;

	g_IsTradeOpen = true;

	let proba = Engine.GuiInterfaceCall("GetTradingGoods", g_ViewedPlayer);
	let button = {};
	let resTradeCodes = g_ResourceData.GetTradableCodes();
	let resBarterCodes = g_ResourceData.GetBarterableCodes();
	let currTradeSelection = resTradeCodes[0];

	let updateTradeButtons = function()
	{
		for (let res in button)
		{
			button[res].label.caption = proba[res] + "%";

			button[res].sel.hidden = !controlsPlayer(g_ViewedPlayer) || res != currTradeSelection;
			button[res].up.hidden = !controlsPlayer(g_ViewedPlayer) || res == currTradeSelection || proba[res] == 100 || proba[currTradeSelection] == 0;
			button[res].dn.hidden = !controlsPlayer(g_ViewedPlayer) || res == currTradeSelection || proba[res] == 0 || proba[currTradeSelection] == 100;
		}
	};

	hideRemaining("tradeResources", resTradeCodes.length);
	Engine.GetGUIObjectByName("tradeHelp").hidden = false;


	for (let i = 0; i < resBarterCodes.length; ++i)
	{
		let resBarterCode = resBarterCodes[i];

		let barterResource = Engine.GetGUIObjectByName("barterResource[" + i + "]");
		if (!barterResource)
		{
			 warn("Current GUI limits prevent displaying more than " + i + " resources in the barter dialog!");
			 break;
		}

		barterOpenCommon(resBarterCode, i, "barter");
		setPanelObjectPosition(barterResource, i, i + 1);
	}

	for (let i = 0; i < resTradeCodes.length; ++i)
	{
		let resTradeCode = resTradeCodes[i];

		let tradeResource = Engine.GetGUIObjectByName("tradeResource[" + i + "]");
		if (!tradeResource)
		{
			 warn("Current GUI limits prevent displaying more than " + i + " resources in the trading goods selection dialog!");
			 break;
		}

		setPanelObjectPosition(tradeResource, i, i + 1);

		let icon = Engine.GetGUIObjectByName("tradeResourceIcon[" + i + "]");
		icon.sprite = "stretched:session/icons/resources/" + resTradeCode + ".png";

		let buttonUp = Engine.GetGUIObjectByName("tradeArrowUp[" + i + "]");
		let buttonDn = Engine.GetGUIObjectByName("tradeArrowDn[" + i + "]");

		button[resTradeCode] = {
			"up": buttonUp,
			"dn": buttonDn,
			"label": Engine.GetGUIObjectByName("tradeResourceText[" + i + "]"),
			"sel": Engine.GetGUIObjectByName("tradeResourceSelection[" + i + "]")
		};

		proba[resTradeCode] = proba[resTradeCode] || 0;

		let buttonResource = Engine.GetGUIObjectByName("tradeResourceButton[" + i + "]");
		buttonResource.enabled = controlsPlayer(g_ViewedPlayer);
		buttonResource.onPress = (resource => {
			return () => {
				if (Engine.HotkeyIsPressed("session.fulltradeswap"))
				{
					for (let res of resTradeCodes)
						proba[res] = 0;
					proba[resource] = 100;
					Engine.PostNetworkCommand({ "type": "set-trading-goods", "tradingGoods": proba });
				}
				currTradeSelection = resource;
				updateTradeButtons();
			};
		})(resTradeCode);

		buttonUp.enabled = controlsPlayer(g_ViewedPlayer);
		buttonUp.onPress = (resource => {
			return () => {
				proba[resource] += Math.min(STEP, proba[currTradeSelection]);
				proba[currTradeSelection] -= Math.min(STEP, proba[currTradeSelection]);
				Engine.PostNetworkCommand({ "type": "set-trading-goods", "tradingGoods": proba });
				updateTradeButtons();
			};
		})(resTradeCode);

		buttonDn.enabled = controlsPlayer(g_ViewedPlayer);
		buttonDn.onPress = (resource => {
			return () => {
				proba[currTradeSelection] += Math.min(STEP, proba[resource]);
				proba[resource] -= Math.min(STEP, proba[resource]);
				Engine.PostNetworkCommand({ "type": "set-trading-goods", "tradingGoods": proba });
				updateTradeButtons();
			};
		})(resTradeCode);
	}

	updateTradeButtons();
	updateTraderTexts();

	Engine.GetGUIObjectByName("tradeDialogPanel").hidden = false;
}

function updateTraderTexts()
{
	let traderNumber = Engine.GuiInterfaceCall("GetTraderNumber", g_ViewedPlayer);
	Engine.GetGUIObjectByName("traderCountText").caption = getIdleLandTradersText(traderNumber) + "\n\n" + getIdleShipTradersText(traderNumber);
}

function initBarterButtons()
{
	let resBartCodes = g_ResourceData.GetBarterableCodes();
	g_BarterSell = resBartCodes.length ? resBartCodes[0] : undefined;
}

/**
 * Code common to both the Barter Panel and the Trade/Barter Dialog, that
 * only needs to be run when the panel or dialog is opened by the player.
 *
 * @param {string} resourceCode
 * @param {number} idx - Element index within its set
 * @param {string} prefix - Common prefix of the gui elements to be worked upon
 */
function barterOpenCommon(resourceCode, idx, prefix)
{
	let barterButton = {};
	for (let action of g_BarterActions)
		barterButton[action] = Engine.GetGUIObjectByName(prefix + action + "Button[" + idx + "]");

	let resource = resourceNameWithinSentence(resourceCode);
	barterButton.Buy.tooltip = sprintf(translate("Buy %(resource)s"), { "resource": resource });
	barterButton.Sell.tooltip = sprintf(translate("Sell %(resource)s"), { "resource": resource });

	barterButton.Sell.onPress = function() {
		g_BarterSell = resourceCode;
		updateSelectionDetails();
		updateBarterButtons();
	};
}

/**
 * Code common to both the Barter Panel and the Trade/Barter Dialog, that
 * needs to be run on simulation update and when relevant hotkeys
 * (i.e. massbarter) are pressed.
 *
 * @param {string} resourceCode
 * @param {number} idx - Element index within its set
 * @param {string} prefix - Common prefix of the gui elements to be worked upon
 * @param {number} player
 */
function barterUpdateCommon(resourceCode, idx, prefix, player)
{
	let barterButton = {};
	let barterIcon = {};
	let barterAmount = {};
	for (let action of g_BarterActions)
	{
		barterButton[action] = Engine.GetGUIObjectByName(prefix + action + "Button[" + idx + "]");
		barterIcon[action] = Engine.GetGUIObjectByName(prefix + action + "Icon[" + idx + "]");
		barterAmount[action] = Engine.GetGUIObjectByName(prefix + action + "Amount[" + idx + "]");
	}
	let selectionIcon = Engine.GetGUIObjectByName(prefix + "SellSelection[" + idx + "]");

	let amountToSell = g_BarterResourceSellQuantity;
	if (Engine.HotkeyIsPressed("session.massbarter"))
		amountToSell *= g_BarterMultiplier;

	let isSelected = resourceCode == g_BarterSell;
	let grayscale = isSelected ? "color:0 0 0 100:grayscale:" : "";

	// Select color of the sell button
	let neededRes = {};
	neededRes[resourceCode] = amountToSell;
	let canSellCurrent = Engine.GuiInterfaceCall("GetNeededResources", {
		"cost": neededRes,
		"player": player
	}) ? "color:255 0 0 80:" : "";

	// Select color of the buy button
	neededRes = {};
	neededRes[g_BarterSell] = amountToSell;
	let canBuyAny = Engine.GuiInterfaceCall("GetNeededResources", {
		"cost": neededRes,
		"player": player
	}) ? "color:255 0 0 80:" : "";

	barterIcon.Sell.sprite = canSellCurrent + "stretched:" + grayscale + "session/icons/resources/" + resourceCode + ".png";
	barterIcon.Buy.sprite = canBuyAny + "stretched:" + grayscale + "session/icons/resources/" + resourceCode + ".png";

	barterAmount.Sell.caption = "-" + amountToSell;
	let prices = GetSimState().players[player].barterPrices;
	barterAmount.Buy.caption = "+" + Math.round(prices.sell[g_BarterSell] / prices.buy[resourceCode] * amountToSell);

	barterButton.Buy.onPress = function() {
		Engine.PostNetworkCommand({
			"type": "barter",
			"sell": g_BarterSell,
			"buy": resourceCode,
			"amount": amountToSell
		});
	};

	barterButton.Buy.hidden = isSelected;
	barterButton.Buy.enabled = controlsPlayer(player);
	barterButton.Sell.hidden = false;
	selectionIcon.hidden = !isSelected;
}

function updateBarterButtons()
{
	let playerState = GetSimState().players[g_ViewedPlayer];
	if (!playerState)
		return;

	let canBarter = playerState.canBarter;
	Engine.GetGUIObjectByName("barterNoMarketsMessage").hidden = canBarter;
	Engine.GetGUIObjectByName("barterResources").hidden = !canBarter;
	Engine.GetGUIObjectByName("barterHelp").hidden = !canBarter;

	if (canBarter)
		g_ResourceData.GetBarterableCodes().forEach((resCode, i) => {
			barterUpdateCommon(resCode, i, "barter", g_ViewedPlayer);
		});
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
				"inactive": translate("%(openingTradingString)s, %(garrisonedString)s, and %(inactiveString)s.")
			},
			"no-garrisoned": {
				"no-inactive": translate("%(openingTradingString)s."),
				"inactive": translate("%(openingTradingString)s, and %(inactiveString)s.")
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

	let inactiveString = sprintf(
		active || garrisoned ?
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

	let garrisonedString = sprintf(
		active || inactive ?
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
		"inactiveString": coloredText(inactiveString, g_IdleTraderTextColor)
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

	let inactiveString = sprintf(
		active ?
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
		"inactiveString": coloredText(inactiveString, g_IdleTraderTextColor)
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

function toggleTutorial()
{
	let tutorialPanel = Engine.GetGUIObjectByName("tutorialPanel");
	tutorialPanel.hidden = !tutorialPanel.hidden ||
	                       !Engine.GetGUIObjectByName("tutorialText").caption;
}

function updateGameSpeedControl()
{
	Engine.GetGUIObjectByName("gameSpeedButton").hidden = g_IsNetworked;

	let player = g_Players[Engine.GetPlayerID()];
	g_GameSpeeds = getGameSpeedChoices(!player || player.state != "active");

	let gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
	gameSpeed.list = g_GameSpeeds.Title;
	gameSpeed.list_data = g_GameSpeeds.Speed;

	let simRate = Engine.GetSimRate();

	let gameSpeedIdx = g_GameSpeeds.Speed.indexOf(+simRate.toFixed(2));
	if (gameSpeedIdx == -1)
		warn("Unknown gamespeed:" + simRate);

	gameSpeed.selected = gameSpeedIdx != -1 ? gameSpeedIdx : g_GameSpeeds.Default;
	gameSpeed.onSelectionChange = function() {
		changeGameSpeed(+this.list_data[this.selected]);
	};
}

function toggleGameSpeed()
{
	let gameSpeed = Engine.GetGUIObjectByName("gameSpeed");
	gameSpeed.hidden = !gameSpeed.hidden;
}

function toggleObjectives()
{
	let open = g_IsObjectivesOpen;
	closeOpenDialogs();

	if (!open)
		openObjectives();
}

function openObjectives()
{
	g_IsObjectivesOpen = true;

	let player = g_Players[Engine.GetPlayerID()];
	let playerState = player && player.state;
	let isActive = !playerState || playerState == "active";

	Engine.GetGUIObjectByName("gameDescriptionText").caption = getGameDescription();

	let objectivesPlayerstate = Engine.GetGUIObjectByName("objectivesPlayerstate");
	objectivesPlayerstate.hidden = isActive;
	objectivesPlayerstate.caption = g_PlayerStateMessages[playerState] || "";

	let gameDescription = Engine.GetGUIObjectByName("gameDescription");
	let gameDescriptionSize = gameDescription.size;
	gameDescriptionSize.top = Engine.GetGUIObjectByName(
		isActive ? "objectivesTitle" : "objectivesPlayerstate").size.bottom;
	gameDescription.size = gameDescriptionSize;

	Engine.GetGUIObjectByName("objectivesPanel").hidden = false;
}

function closeObjectives()
{
	g_IsObjectivesOpen = false;
	Engine.GetGUIObjectByName("objectivesPanel").hidden = true;
}

/**
 * Allows players to see their own summary.
 * If they have shared ally vision researched, they are able to see the summary of there allies too.
 */
function openGameSummary()
{
	closeOpenDialogs();
	pauseGame();

	let extendedSimState = Engine.GuiInterfaceCall("GetExtendedSimulationState");
	Engine.PushGuiPage(
		"page_summary.xml",
		{
			"sim": {
				"mapSettings": g_GameAttributes.settings,
				"playerStates": extendedSimState.players.filter((state, player) =>
					g_IsObserver || player == 0 || player == g_ViewedPlayer ||
					extendedSimState.players[g_ViewedPlayer].hasSharedLos && g_Players[player].isMutualAlly[g_ViewedPlayer]),
				"timeElapsed": extendedSimState.timeElapsed
			},
			"gui": {
				"dialog": true,
				"isInGame": true
			},
			"selectedData": g_SummarySelectedData
		},
		resumeGameAndSaveSummarySelectedData);
}

function openStrucTree(page)
{
	closeOpenDialogs();
	pauseGame();

	Engine.PushGuiPage(
		page,
		{
			"civ": g_CivInfo.civ || g_Players[g_ViewedPlayer].civ
			// TODO add info about researched techs and unlocked entities
		},
		storeCivInfoPage);
}

function storeCivInfoPage(data)
{
	if (data.nextPage)
		Engine.PushGuiPage(
			data.nextPage,
			{ "civ": data.civ },
			storeCivInfoPage);
	else
	{
		g_CivInfo = data;
		resumeGame();
	}
}

/**
 * Pause or resume the game.
 *
 * @param explicit - true if the player explicitly wants to pause or resume.
 * If this argument isn't set, a multiplayer game won't be paused and the pause overlay
 * won't be shown in single player.
 */
function pauseGame(pause = true, explicit = false)
{
	// The NetServer only supports pausing after all clients finished loading the game.
	if (g_IsNetworked && (!explicit || !g_IsNetworkedActive))
		return;

	if (explicit)
		g_Paused = pause;

	Engine.SetPaused(g_Paused || pause, !!explicit);

	if (g_IsNetworked)
	{
		setClientPauseState(Engine.GetPlayerGUID(), g_Paused);
		return;
	}

	updatePauseOverlay();
}

function resumeGame(explicit = false)
{
	pauseGame(false, explicit);
}

function resumeGameAndSaveSummarySelectedData(data)
{
	g_SummarySelectedData = data.summarySelectedData;
	resumeGame(data.explicitResume);
}

/**
 * Called when the current player toggles a pause button.
 */
function togglePause()
{
	if (!Engine.GetGUIObjectByName("pauseButton").enabled)
		return;

	closeOpenDialogs();

	pauseGame(!g_Paused, true);
}

/**
 * Called when a client pauses or resumes in a multiplayer game.
 */
function setClientPauseState(guid, paused)
{
	// Update the list of pausing clients.
	let index = g_PausingClients.indexOf(guid);
	if (paused && index == -1)
		g_PausingClients.push(guid);
	else if (!paused && index != -1)
		g_PausingClients.splice(index, 1);

	updatePauseOverlay();

	Engine.SetPaused(!!g_PausingClients.length, false);
}

/**
 * Update the pause overlay.
 */
function updatePauseOverlay()
{
	Engine.GetGUIObjectByName("pauseButton").caption = g_Paused ? translate("Resume") : translate("Pause");
	Engine.GetGUIObjectByName("resumeMessage").hidden = !g_Paused;

	Engine.GetGUIObjectByName("pausedByText").hidden = !g_IsNetworked;
	Engine.GetGUIObjectByName("pausedByText").caption = sprintf(translate("Paused by %(players)s"),
		{ "players": g_PausingClients.map(guid => colorizePlayernameByGUID(guid)).join(translateWithContext("Separator for a list of players", ", ")) });

	Engine.GetGUIObjectByName("pauseOverlay").hidden = !(g_Paused || g_PausingClients.length);
	Engine.GetGUIObjectByName("pauseOverlay").onPress = g_Paused ? togglePause : function() {};
}

function openManual()
{
	closeOpenDialogs();
	pauseGame();
	Engine.PushGuiPage("page_manual.xml", {}, resumeGame);
}

function closeOpenDialogs()
{
	closeMenu();
	closeChat();
	closeDiplomacy();
	closeTrade();
	closeObjectives();
}

function formatTributeTooltip(playerID, resourceCode, amount)
{
	return sprintf(translate("Tribute %(resourceAmount)s %(resourceType)s to %(playerName)s. Shift-click to tribute %(greaterAmount)s."), {
		"resourceAmount": amount,
		"resourceType": resourceNameWithinSentence(resourceCode),
		"playerName": colorizePlayernameByID(playerID),
		"greaterAmount": amount < 500 ? 500 : amount + 500
	});
}
