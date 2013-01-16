const PAUSE = "Pause";
const RESUME = "Resume";

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

var isMenuOpen = false;
var menu;

var isDiplomacyOpen = false;

// Ignore size defined in XML and set the actual menu size here
function initMenuPosition()
{
	menu = getGUIObjectByName("menu");
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
//	playButtonSound();
	isMenuOpen = true;
}

// Closes the menu and resets position
function closeMenu()
{
//	playButtonSound();
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
	openSettings(true);
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
	var btCaptions = ["Yes", "No"];
	var btCode = [resignGame, resumeGame];
	messageBox(400, 200, "Are you sure you want to resign?", "Confirmation", 0, btCaptions, btCode);
}

function exitMenuButton()
{
	closeMenu();
	closeOpenDialogs();
	pauseGame();
	var btCaptions = ["Yes", "No"];
	var btCode = [leaveGame, resumeGame];
	messageBox(400, 200, "Are you sure you want to quit?", "Confirmation", 0, btCaptions, btCode);
}

function openDeleteDialog(selection)
{
	closeMenu();
	closeOpenDialogs();

	var deleteSelectedEntities = function ()
	{
		Engine.PostNetworkCommand({"type": "delete-entities", "entities": selection});
	};

	var btCaptions = ["Yes", "No"];
	var btCode = [deleteSelectedEntities, resumeGame];

	messageBox(400, 200, "Destroy everything currently selected?", "Delete", 0, btCaptions, btCode);
}

// Menu functions
// =============================================================================

function openSettings(pause)
{
	getGUIObjectByName("settingsDialogPanel").hidden = false;
	if (pause)
		pauseGame();
}

function closeSettings(resume)
{
	getGUIObjectByName("settingsDialogPanel").hidden = true;
	if (resume)
		resumeGame();
}

function openChat()
{
	getGUIObjectByName("chatInput").focus(); // Grant focus to the input area
	getGUIObjectByName("chatDialogPanel").hidden = false;

}

function closeChat()
{
	getGUIObjectByName("chatInput").caption = ""; // Clear chat input
	getGUIObjectByName("chatInput").blur(); // Remove focus
	getGUIObjectByName("chatDialogPanel").hidden = true;
}

function toggleChatWindow(teamChat)
{
	closeSettings();

	var chatWindow = getGUIObjectByName("chatDialogPanel");
	var chatInput = getGUIObjectByName("chatInput");

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

	getGUIObjectByName("toggleTeamChat").checked = teamChat;
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
	isDiplomacyOpen = true;

	var we = Engine.GetPlayerID();
	var players = getPlayerData(g_PlayerAssignments);

	// Get offset for one line
	var onesize = getGUIObjectByName("diplomacyPlayer[0]").size;
	var rowsize = onesize.bottom - onesize.top;

	// We don't include gaia
	for (var i = 1; i < players.length; i++)
	{
		// Apply offset
		var row = getGUIObjectByName("diplomacyPlayer["+(i-1)+"]");
		var size = row.size;
		size.top = rowsize*(i-1);
		size.bottom = rowsize*i;
		row.size = size;

		// Set background colour
		var playerColor = players[i].color.r+" "+players[i].color.g+" "+players[i].color.b;
		row.sprite = "colour: "+playerColor + " 32";

		getGUIObjectByName("diplomacyPlayerName["+(i-1)+"]").caption = "[color=\"" + playerColor + "\"]" + players[i].name + "[/color]";
		getGUIObjectByName("diplomacyPlayerCiv["+(i-1)+"]").caption = g_CivData[players[i].civ].Name;

		getGUIObjectByName("diplomacyPlayerTeam["+(i-1)+"]").caption = (players[i].team < 0) ? "None" : players[i].team+1;

		if (i != we)
			getGUIObjectByName("diplomacyPlayerTheirs["+(i-1)+"]").caption = (players[i].isAlly[we] ? "Ally" : (players[i].isNeutral[we] ? "Neutral" : "Enemy"));

		// Don't display the options for ourself, or if we or the other player aren't active anymore
		if (i == we || players[we].state != "active" || players[i].state != "active")
		{
			// Hide the unused/unselectable options
			for each (var a in ["TributeFood", "TributeWood", "TributeStone", "TributeMetal", "Ally", "Neutral", "Enemy"])
				getGUIObjectByName("diplomacyPlayer"+a+"["+(i-1)+"]").hidden = true;
			continue;
		}

		// Tribute
		for each (var resource in ["food", "wood", "stone", "metal"])
		{
			var button = getGUIObjectByName("diplomacyPlayerTribute"+toTitleCase(resource)+"["+(i-1)+"]");
			// TODO: Make amounts changeable or change to 500 if shift is pressed
			var amounts = {
				"food": (resource=="food")?100:0,
				"wood": (resource=="wood")?100:0,
				"stone": (resource=="stone")?100:0,
				"metal": (resource=="metal")?100:0,
			};
			button.onpress = (function(e){ return function() { tributeResource(e) } })({"player": i, "amounts": amounts});
			button.hidden = false;
		}

		// Skip our own teams on teams locked
		if (players[we].teamsLocked && players[we].team != -1 && players[we].team == players[i].team)
			continue;

		// Diplomacy settings
		// Set up the buttons
		for each (var setting in ["ally", "neutral", "enemy"])
		{
			var button = getGUIObjectByName("diplomacyPlayer"+toTitleCase(setting)+"["+(i-1)+"]");

			if (setting == "ally")
			{
				if (players[we].isAlly[i])
					button.caption = "x";
				else
					button.caption = "";
			}
			else if (setting == "neutral")
			{
				if (players[we].isNeutral[i])
					button.caption = "x";
				else
					button.caption = "";
			}
			else // "enemy"
			{
				if (players[we].isEnemy[i])
					button.caption = "x";
				else
					button.caption = "";
			}
			
			button.onpress = (function(e){ return function() { setDiplomacy(e) } })({"player": i, "to": setting});
			button.hidden = false;
		}
	}

	getGUIObjectByName("diplomacyDialogPanel").hidden = false;
}

function closeDiplomacy()
{
	isDiplomacyOpen = false;
	getGUIObjectByName("diplomacyDialogPanel").hidden = true;
}

function toggleDiplomacy()
{
	if (isDiplomacyOpen)
		closeDiplomacy();
	else
		openDiplomacy();
};

function pauseGame()
{
	getGUIObjectByName("pauseButtonText").caption = RESUME;
	getGUIObjectByName("pauseOverlay").hidden = false;
	setPaused(true);
}

function resumeGame()
{
	getGUIObjectByName("pauseButtonText").caption = PAUSE;
	getGUIObjectByName("pauseOverlay").hidden = true;
	setPaused(false);
}

function togglePause()
{
	closeMenu();
	closeOpenDialogs();

	var pauseOverlay = getGUIObjectByName("pauseOverlay");

	if (pauseOverlay.hidden)
	{
		getGUIObjectByName("pauseButtonText").caption = RESUME;
		setPaused(true);
	}
	else
	{
		setPaused(false);
		getGUIObjectByName("pauseButtonText").caption = PAUSE;
	}

	pauseOverlay.hidden = !pauseOverlay.hidden;
}

function openManual()
{
	closeMenu();
	closeOpenDialogs();
	pauseGame();
	Engine.PushGuiPage("page_manual.xml", {"page": "intro", "closeCallback": resumeGame});
}

function toggleDeveloperOverlay()
{
	var devCommands = getGUIObjectByName("devCommands");
	var text = devCommands.hidden ? "opened." : "closed.";
	submitChatDirectly("The Developer Overlay was " + text);
	// Update the options dialog
	getGUIObjectByName("developerOverlayCheckbox").checked = devCommands.hidden;
	devCommands.hidden = !devCommands.hidden;
}

function closeOpenDialogs()
{
	closeMenu();
	closeChat();
	closeDiplomacy();
	closeSettings(false);
}
