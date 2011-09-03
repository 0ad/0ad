var userReportEnabledText; // contains the original version with "$status" placeholder
var currentSubmenuType; // contains submenu type
const MARGIN = 4; // menu border size

function init()
{
	global.curr_music = newRandomSound("music", "menu");
	if (global.curr_music)
		global.curr_music.loop();

	userReportEnabledText = getGUIObjectByName("userReportEnabledText").caption;

        // initialize currentSubmenuType with placeholder to avoid null
        currentSubmenuType = "submenuSinglePlayer";
}

var t0 = new Date;
function scrollBackgrounds()
{
	var layer1 = getGUIObjectByName("backgroundLayer1");
	var layer2 = getGUIObjectByName("backgroundLayer2");
	var layer3 = getGUIObjectByName("backgroundLayer3");

	var screen = layer1.parent.getComputedSize();
	var h = screen.bottom - screen.top; // height of screen
	var w = h*16/9; // width of background image

	// Offset the layers by oscillating amounts
	var t = (t0 - new Date) / 1000;
	var speed = 1/20;
	var off1 = 0.02 * w * (1+Math.cos(t*speed));
	var off2 = 0.12 * w * (1+Math.cos(t*speed)) - h*6/9;
	var off3 = 0.16 * w * (1+Math.cos(t*speed));

	var left = screen.right - w * (1 + Math.ceil(screen.right / w));
	layer1.size = new GUISize(left + off1, screen.top, screen.right + off1, screen.bottom);
	layer2.size = new GUISize(screen.right/2 - h + off2, screen.top, screen.right/2 + h + off2, screen.bottom);
	layer3.size = new GUISize(screen.right - h + off3, screen.top, screen.right + off3, screen.bottom);
}

function submitUserReportMessage()
{
	var input = getGUIObjectByName("userReportMessageInput");
	var msg = input.caption;
	if (msg.length)
		Engine.SubmitUserReport("message", 1, msg);
	input.caption = "";
}

function formatUserReportStatus(status)
{
	var d = status.split(/:/, 3);

	if (d[0] == "disabled")
		return "disabled";

	if (d[0] == "connecting")
		return "connecting to server";

	if (d[0] == "sending")
	{
		var done = d[1];
		return "uploading (" + Math.floor(100*done) + "%)";
	}

	if (d[0] == "completed")
	{
		var httpCode = d[1];
		if (httpCode == 200)
			return "upload succeeded";
		else
			return "upload failed (" + httpCode + ")";
	}

	if (d[0] == "failed")
	{
		var errCode = d[1];
		var errMessage = d[2];
		return "upload failed (" + errMessage + ")";
	}

	return "unknown";
}

function onTick()
{
        // Animate backgrounds
	scrollBackgrounds();

        // Animate submenu
        updateMenuPosition();

	if (Engine.IsUserReportEnabled())
	{
		getGUIObjectByName("userReportDisabled").hidden = true;
		getGUIObjectByName("userReportEnabled").hidden = false;

		getGUIObjectByName("userReportEnabledText").caption =
			userReportEnabledText.replace(/\$status/,
				formatUserReportStatus(Engine.GetUserReportStatus()));
	}
	else
	{
		getGUIObjectByName("userReportDisabled").hidden = false;
		getGUIObjectByName("userReportEnabled").hidden = true;
	}
}


/*
 * MENU FUNCTIONS
 */

// Slide menu
function updateMenuPosition()
{
        var submenu = getGUIObjectByName("submenu");

        if (submenu.hidden == false)
        {
                // The offset is the increment or number of units/pixels to move
                // the menu. An offset of one is always accurate, but it is too
                // slow. The offset must divide into the travel distance evenly
                // in order for the menu to end up at the right spot. The travel
                // distance is the max-initial. The travel distance in this
                // example is 300-60 = 240. We choose an offset of 5 because it
                // divides into 240 evenly and provided the speed we wanted.
                var OFFSET = 10;

                if (submenu.size.left < getGUIObjectByName("mainMenu").size.right)
                {
                        submenu.size = (submenu.size.left + OFFSET) + " " + submenu.size.top + " " + (submenu.size.right + OFFSET) + " " + submenu.size.bottom;
                }
        }
}

// Opens the menu by revealing the screen which contains the menu
function openMenu(newSubmenu, position, buttonHeight, numButtons)
{
        var menuSound = new newRandomSound("effect", "arrowfly_", "audio/attack/weapon");
	if (menuSound)
	{
		menuSound.play(0);
	}

        // switch to new submenu type
        currentSubmenuType = newSubmenu;
        getGUIObjectByName(currentSubmenuType).hidden = false;

        // set position of new submenu
        var submenu = getGUIObjectByName("submenu");
        var top = position - MARGIN;
        var bottom = position + ((buttonHeight + MARGIN) * numButtons);
        submenu.size = submenu.size.left + " " + top + " " + submenu.size.right + " " + bottom;

        // Blend in right border of main menu into the left border of the submenu
        blendSubmenuIntoMain(top, bottom);

        // Reveal submenu
        getGUIObjectByName("submenu").hidden = false;

        // prepare to hide the submenu when the user clicks on the background
        getGUIObjectByName("submenuScreen").hidden = false;
}

// Closes the menu and resets position
function closeMenu()
{
        // remove old submenu type
        getGUIObjectByName(currentSubmenuType).hidden = true;

        // hide submenu and reset position
        var submenu = getGUIObjectByName("submenu");
        submenu.hidden = true;
        submenu.size = getGUIObjectByName("mainMenu").size;

        // reset main menu panel right border
        getGUIObjectByName("MainMenuPanelRightBorderTop").size = "100%-2 0 100% 100%";

        // hide submenu screen
        getGUIObjectByName("submenuScreen").hidden = false;
        console.write(getGUIObjectByName("submenuScreen").hidden);
}

// Sizes right border on main menu panel to match the submenu
function blendSubmenuIntoMain(topPosition, bottomPosition)
{
    var topSprite = getGUIObjectByName("MainMenuPanelRightBorderTop");
    topSprite.size = "100%-2 0 100% " + (topPosition + MARGIN);

    var bottomSprite = getGUIObjectByName("MainMenuPanelRightBorderBottom");
    bottomSprite.size = "100%-2 " + (bottomPosition) + " 100% 100%";
}

// Reveals submenu
function openMainMenuSubWindow (windowName)
{
	guiUnHide("pgSubWindow");
	guiUnHide(windowName);
}

// Hides submenu
function closeMainMenuSubWindow (windowName)
{
	guiHide("pgSubWindow");
	guiHide(windowName);
}



/*
 * FUNCTIONS BELOW DO NOT WORK YET
 */

// Switch to a given options tab window.
function openOptionsTab(tabName)
{
	// Hide the other tabs.
	for (i = 1; i <= 3; i++)
	{
		switch (i)
		{
			case 1:
				var tmpName = "pgOptionsAudio";
			break;
			case 2:
				var tmpName = "pgOptionsVideo";
			break;
			case 3:
				var tmpName = "pgOptionsGame";
			break;
			default:
			break;
		}

		if (tmpName != tabName)
		{
			getGUIObjectByName (tmpName + "Window").hidden = true;
			getGUIObjectByName (tmpName + "Button").enabled = true;
		}
	}

	// Make given tab visible.
	getGUIObjectByName (tabName + "Window").hidden = false;
	getGUIObjectByName (tabName + "Button").enabled = false;
}

// Move the credits up the screen.
function updateCredits()
{
	// If there are still credit lines to remove, remove them.
	if (getNumItems("pgCredits") > 0)
		removeItem ("pgCredits", 0);
	else
	{
		// When we've run out of credit,

		// Stop the increment timer if it's still active.
		cancelInterval();

		// Close the credits screen and return.
		closeMainMenuSubWindow ("pgCredits");
		guiUnHide ("pg");
	}
}
