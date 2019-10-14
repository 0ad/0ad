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

/**
 * Store civilization code and page (structree or history) opened in civilization info.
 */
var g_CivInfo = {
	"civ": "",
	"page": "page_structree.xml"
};

var g_IsMenuOpen = false;

var g_IsObjectivesOpen = false;

/**
 * Remember last viewed summary panel and charts.
 */
var g_SummarySelectedData;

function initMenu()
{
	Engine.GetGUIObjectByName("menu").size = "100%-164 " + MENU_TOP + " 100% " + MENU_BOTTOM;

	// TODO: Atlas should pass g_GameAttributes.settings
	for (let button of ["menuExitButton", "summaryButton", "objectivesButton"])
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
	g_Chat.openPage();
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
	closeObjectives();

	g_Chat.closePage();
	g_DiplomacyDialog.close();
	g_TradeDialog.close();
}
