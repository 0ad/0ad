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

// Menu starting position - bottom
const MENU_BOTTOM = 36;

// Menu starting position - top
const MENU_TOP = MENU_BOTTOM - END_MENU_POSITION;

// Menu starting position - overall
const INITIAL_MENU_POSITION = "100%-164 " + MENU_TOP + " 100% " + MENU_BOTTOM;


// Slide menu
function updateMenuPosition()
{
        if (getGUIObjectByName("menu").hidden == false)
        {
                var menu = getGUIObjectByName("menu");

                // The offset is the increment or number of units/pixels to move
                // the menu. An offset of one is always accurate, but it is too
                // slow. The offset must divide into the travel distance evenly
                // in order for the menu to end up at the right spot. The travel
                // distance is the max-initial. The travel distance in this
                // example is 196-36 = 160. We choose an offset of 16 because it
                // divides into 160 evenly and provided the speed we wanted.
                var OFFSET = 16;

                if (menu.size.bottom < END_MENU_POSITION)
                {
                        menu.size = "100%-164 " + (menu.size.top + OFFSET) + " 100% " + (menu.size.bottom + OFFSET);
                }
        }
}

// Opens the menu by revealing the screen which contains the menu
function openMenu()
{
        var menuSound = new Sound("audio/interface/ui/ui_button_longclick.ogg");
	if (menuSound)
	{
		menuSound.play(0);
	}

        getGUIObjectByName("menu").hidden = false;
}

// Closes the menu and resets position
function closeMenu()
{
        getGUIObjectByName("menu").hidden = true;
        getGUIObjectByName("menu").size = INITIAL_MENU_POSITION;
}

function openSettings()
{
        closeMenu();
        closeChat();
        getGUIObjectByName("settingsDialogPanel").hidden = false;
}

function closeSettings()
{
        getGUIObjectByName("settingsDialogPanel").hidden = true;
}

function openChat()
{
        closeMenu();
        closeSettings();
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

function togglePause()
{
        closeMenu();
        closeChat();
        closeSettings();

	var pauseOverlay = getGUIObjectByName("pauseOverlay");

	if (pauseOverlay.hidden)
	{
		setPaused(true);
		getGUIObjectByName("pauseButtonText").caption = "Unpause";
	}
	else
	{
		setPaused(false);
		getGUIObjectByName("pauseButtonText").caption = "Pause";
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
        closeSettings();
}