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

/**
 * Remember last viewed summary panel and charts.
 */
var g_SummarySelectedData;

function initMenu(playerViewControl, pauseControl)
{
	Engine.GetGUIObjectByName("menu").size = "100%-164 " + MENU_TOP + " 100% " + MENU_BOTTOM;
	Engine.GetGUIObjectByName("lobbyButton").enabled = Engine.HasXmppClient();
	playerViewControl.registerViewedPlayerChangeHandler(updateResignButton);
	updatePauseButton();
	pauseControl.registerPauseHandler(updatePauseButton);

	// TODO: Atlas should pass g_GameAttributes.settings
	for (let button of ["menuExitButton", "summaryButton"])
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

function updatePauseButton()
{
	let pauseButton = Engine.GetGUIObjectByName("pauseButton");
	pauseButton.caption = g_PauseControl.explicitPause ? translate("Resume") : translate("Pause");
	pauseButton.enabled = !g_IsObserver || !g_IsNetworked || g_IsController;
	pauseButton.onPress = () => {
		closeMenu();
		g_PauseControl.setPaused(!g_PauseControl.explicitPause, true);
	};
}

function updateResignButton()
{
	Engine.GetGUIObjectByName("menuResignButton").enabled = !g_IsObserver;
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
	g_PauseControl.implicitPause();

	messageBox(
		400, 200,
		translate("Are you sure you want to resign?"),
		translate("Confirmation"),
		[translate("No"), translate("Yes")],
		[resumeGame, resignGame]);
}

function exitMenuButton()
{
	closeOpenDialogs();
	g_PauseControl.implicitPause();

	let messageTypes = {
		"host": {
			"caption": translate("Are you sure you want to quit? Leaving will disconnect all other players."),
			"buttons": [resumeGame, endGame]
		},
		"client": {
			"caption": translate("Are you sure you want to quit?"),
			"buttons": [resumeGame, resignQuestion]
		},
		"singleplayer": {
			"caption": translate("Are you sure you want to quit?"),
			"buttons": [resumeGame, endGame]
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
		[endGame, resignGame]);
}

function openDeleteDialog(selection)
{
	closeOpenDialogs();
	g_PauseControl.implicitPause();

	let deleteSelectedEntities = function(selectionArg)
	{
		Engine.PostNetworkCommand({
			"type": "delete-entities",
			"entities": selectionArg
		});
		resumeGame();
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
	g_PauseControl.implicitPause();

	Engine.PushGuiPage(
		"page_loadgame.xml",
		{ "savedGameData": getSavedGameData() },
		resumeGame);
}

function openOptions()
{
	closeOpenDialogs();
	g_PauseControl.implicitPause();

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

/**
 * Allows players to see their own summary.
 * If they have shared ally vision researched, they are able to see the summary of there allies too.
 */
function openGameSummary()
{
	closeOpenDialogs();
	g_PauseControl.implicitPause();

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
		data =>
		{
			g_SummarySelectedData = data.summarySelectedData;
			g_PauseControl.implicitResume();
		});
}

function openStrucTree(page)
{
	closeOpenDialogs();
	g_PauseControl.implicitPause();

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

function openManual()
{
	closeOpenDialogs();
	g_PauseControl.implicitPause();
	Engine.PushGuiPage("page_manual.xml", {}, resumeGame);
}

function closeOpenDialogs()
{
	closeMenu();
	g_Chat.closePage();
	g_DiplomacyDialog.close();
	g_ObjectivesDialog.close();
	g_TradeDialog.close();
}
