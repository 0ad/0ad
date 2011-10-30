const PAUSE = "Pause";
const RESUME = "Resume";

/*
 * MENU POSITION CONSTANTS
*/

// Menu / panel border size
const MARGIN = 4;

// Includes the main menu button
const NUM_BUTTONS = 6;

// Regular menu buttons
const BUTTON_HEIGHT = 32;

// The position where the bottom of the menu will end up (currently 164)
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

function pauseMenuButton()
{
	togglePause();
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
	getGUIObjectByName("chatDialogPanel").hidden = true;
}

function toggleChatWindow()
{
	closeSettings();

	var chatWindow = getGUIObjectByName("chatDialogPanel");
	var chatInput = getGUIObjectByName("chatInput");

	if (chatWindow.hidden)
		chatInput.focus(); // Grant focus to the input area
	else
		chatInput.caption = ""; // Clear chat input

	chatWindow.hidden = !chatWindow.hidden;
}

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

function toggleDeveloperOverlay()
{
	var devCommands = getGUIObjectByName("devCommands");
	var text = devCommands.hidden? "opened." : "closed.";
	submitChatDirectly("The Developer Overlay was " + text);
	devCommands.hidden = !devCommands.hidden;
}

function closeOpenDialogs()
{
	closeMenu();
	closeChat();
	closeSettings(false);
}


// Temporarily adding this here
//function playButtonSound()
//{
//    const BUTTON_SOUND = "audio/interface/ui/ui_button_longclick.ogg";
//    var buttonSound = new Sound(BUTTON_SOUND);
//    buttonSound.play();
//}