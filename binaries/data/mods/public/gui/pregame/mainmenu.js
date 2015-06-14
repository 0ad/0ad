var userReportEnabledText; // contains the original version with "$status" placeholder
var currentSubmenuType; // contains submenu type
const MARGIN = 4; // menu border size
var g_BackgroundCode; // Background type.

var g_ShowSplashScreens;
var g_BackgroundLayerData = [];

function init(initData, hotloadData)
{
	initMusic();
	// Play main menu music
	global.music.setState(global.music.states.MENU);

	userReportEnabledText = Engine.GetGUIObjectByName("userReportEnabledText").caption;

	// initialize currentSubmenuType with placeholder to avoid null when switching
	currentSubmenuType = "submenuSinglePlayer";

	EnableUserReport(Engine.IsUserReportEnabled());

	// Only show splash screen(s) once at startup, but not again after hotloading
	g_ShowSplashScreens = hotloadData ? hotloadData.showSplashScreens : initData && initData.isStartup;

	// Pick a random background and initialise it
	g_BackgroundCode = Math.floor(Math.random() * g_BackgroundLayerData.length);
	var layerset = g_BackgroundLayerData[g_BackgroundCode];
	for (var i = 0; i < layerset.length; ++i)
	{
		var layer = layerset[i];
		var guiObj = Engine.GetGUIObjectByName("background["+i+"]");
		guiObj.hidden = false;
		guiObj.sprite = layer.sprite;
		guiObj.z = i;
	}
}

function getHotloadData()
{
	return { "showSplashScreens": g_ShowSplashScreens };
}

var t0 = +(new Date());
function scrollBackgrounds()
{
	var layerset = g_BackgroundLayerData[g_BackgroundCode];
	for (var i = 0; i < layerset.length; i++)
	{
		var layer = layerset[i];
		var guiObj = Engine.GetGUIObjectByName("background["+i+"]");

		var screen = guiObj.parent.getComputedSize();
		var h = screen.bottom - screen.top;
		var w = h * 16/9;

		var time = (new Date() - t0) / 1000;
		var offset = layer.offset(time, w);
		if (layer.tiling)
		{
			var left = offset % screen.right;
			if (left > 0)
				left -= screen.right;
			var right = left + screen.right * 2;
			guiObj.size = new GUISize(left, screen.top, right, screen.bottom);
		}
		else
			guiObj.size = new GUISize(screen.right/2 - h + offset, screen.top, screen.right/2 + h + offset, screen.bottom);
	}
}

function submitUserReportMessage()
{
	var input = Engine.GetGUIObjectByName("userReportMessageInput");
	var msg = input.caption;
	if (msg.length)
		Engine.SubmitUserReport("message", 1, msg);
	input.caption = "";
}

function formatUserReportStatus(status)
{
	var d = status.split(/:/, 3);

	if (d[0] == "disabled")
		return translate("disabled");

	if (d[0] == "connecting")
		return translate("connecting to server");

	if (d[0] == "sending")
	{
		var done = d[1];
		return sprintf(translate("uploading (%f%%)"), Math.floor(100*done));
	}

	if (d[0] == "completed")
	{
		var httpCode = d[1];
		if (httpCode == 200)
			return translate("upload succeeded");
		else
			return sprintf(translate("upload failed (%(errorCode)s)"), { errorCode: httpCode });
	}

	if (d[0] == "failed")
	{
		var errCode = d[1];
		var errMessage = d[2];
		return sprintf(translate("upload failed (%(errorMessage)s)"), { errorMessage: errMessage });
	}

	return translate("unknown");
}

var lastTickTime = new Date;
function onTick()
{
	var now = new Date;
	var tickLength = new Date - lastTickTime;
	lastTickTime = now;

	// Animate backgrounds
	scrollBackgrounds();

	// Animate submenu
	updateMenuPosition(tickLength);

	if (Engine.IsUserReportEnabled())
	{
		Engine.GetGUIObjectByName("userReportEnabledText").caption = 
			userReportEnabledText.replace(/\$status/,
				formatUserReportStatus(Engine.GetUserReportStatus()));
	}

	// Show splash screens here, so we don't interfere with main menu hotloading
	if (g_ShowSplashScreens)
	{
		g_ShowSplashScreens = false;

		if (Engine.ConfigDB_GetValue("user", "splashscreendisable") !== "true" 
			&& Engine.ConfigDB_GetValue("user", "splashscreenversion") < Engine.GetFileMTime("gui/splashscreen/splashscreen.txt"))
            Engine.PushGuiPage("page_splashscreen.xml", { "page": "splashscreen", callback : "SplashScreenClosedCallback" } );
		else
			ShowRenderPathMessage();
	}
}

function ShowRenderPathMessage()
{
	// Warn about removing fixed render path
	if (Engine.Renderer_GetRenderPath() == "fixed")
		messageBox(
			600,
			300,
			"[font=\"sans-bold-16\"]" +
			sprintf(translate("%(startWarning)sWarning:%(endWarning)s You appear to be using non-shader (fixed function) graphics. This option will be removed in a future 0 A.D. release, to allow for more advanced graphics features. We advise upgrading your graphics card to a more recent, shader-compatible model."), { startWarning: "[color=\"200 20 20\"]", endWarning: "[/color]"}) +
			"\n\n" +
			// Translation: This is the second paragraph of a warning. The
			// warning explains that the user is using “non-shader“ graphics,
			// and that in the future this will not be supported by the game, so
			// the user will need a better graphics card.
			translate("Please press \"Read More\" for more information or \"OK\" to continue."),
			translate("WARNING!"),
			0,
			[translate("OK"), translate("Read More")],
			[ null, function() { Engine.OpenURL("http://www.wildfiregames.com/forum/index.php?showtopic=16734"); } ]
		);
}

function SplashScreenClosedCallback()
{
	ShowRenderPathMessage();
}

function EnableUserReport(Enabled)
{
	Engine.GetGUIObjectByName("userReportDisabled").hidden = Enabled;
 	Engine.GetGUIObjectByName("userReportEnabled").hidden = !Enabled;
 	Engine.SetUserReportEnabled(Enabled);
}


/*
 * MENU FUNCTIONS
 */

// Slide menu
function updateMenuPosition(dt)
{
	var submenu = Engine.GetGUIObjectByName("submenu");

	if (submenu.hidden == false)
	{
		// Number of pixels per millisecond to move
		const SPEED = 1.2;

		var maxOffset = Engine.GetGUIObjectByName("mainMenu").size.right - submenu.size.left;
		if (maxOffset > 0)
		{
			var offset = Math.min(SPEED * dt, maxOffset);
			var size = submenu.size;
			size.left += offset;
			size.right += offset;
			submenu.size = size;
		}
	}
}

// Opens the menu by revealing the screen which contains the menu
function openMenu(newSubmenu, position, buttonHeight, numButtons)
{
	// switch to new submenu type
	currentSubmenuType = newSubmenu;
	Engine.GetGUIObjectByName(currentSubmenuType).hidden = false;

	// set position of new submenu
	var submenu = Engine.GetGUIObjectByName("submenu");
	var top = position - MARGIN;
	var bottom = position + ((buttonHeight + MARGIN) * numButtons);
	submenu.size = submenu.size.left + " " + top + " " + submenu.size.right + " " + bottom;

	// Blend in right border of main menu into the left border of the submenu
	blendSubmenuIntoMain(top, bottom);

	// Reveal submenu
	Engine.GetGUIObjectByName("submenu").hidden = false;
}

// Closes the menu and resets position
function closeMenu()
{
//	playButtonSound();

	// remove old submenu type
	Engine.GetGUIObjectByName(currentSubmenuType).hidden = true;

	// hide submenu and reset position
	var submenu = Engine.GetGUIObjectByName("submenu");
	submenu.hidden = true;
	submenu.size = Engine.GetGUIObjectByName("mainMenu").size;

	// reset main menu panel right border
	Engine.GetGUIObjectByName("MainMenuPanelRightBorderTop").size = "100%-2 0 100% 100%";
}

// Sizes right border on main menu panel to match the submenu
function blendSubmenuIntoMain(topPosition, bottomPosition)
{
	var topSprite = Engine.GetGUIObjectByName("MainMenuPanelRightBorderTop");
	topSprite.size = "100%-2 0 100% " + (topPosition + MARGIN);

	var bottomSprite = Engine.GetGUIObjectByName("MainMenuPanelRightBorderBottom");
	bottomSprite.size = "100%-2 " + (bottomPosition) + " 100% 100%";
}

function getBuildString()
{
	return sprintf(translate("Build: %(buildDate)s (%(revision)s)"), { buildDate: Engine.GetBuildTimestamp(0), revision: Engine.GetBuildTimestamp(2) });
}

/*
 * FUNCTIONS BELOW DO NOT WORK YET
 */

//// Move the credits up the screen.
//function updateCredits()
//{
//	// If there are still credit lines to remove, remove them.
//	if (getNumItems("pgCredits") > 0)
//		removeItem ("pgCredits", 0);
//	else
//	{
//		// When we've run out of credit,
//
//		// Stop the increment timer if it's still active.
//		cancelInterval();
//
//		// Close the credits screen and return.
//		closeMainMenuSubWindow ("pgCredits");
//		guiUnHide ("pg");
//	}
//}

function exitGamePressed()
{
	closeMenu();
	var btCaptions = [translate("No"), translate("Yes")];
	var btCode = [null, Engine.Exit];
	messageBox(400, 200, translate("Are you sure you want to quit 0 A.D.?"), translate("Confirmation"), 0, btCaptions, btCode);
}

function pressedScenarioEditorButton()
{
	closeMenu();
	// Start Atlas
	if (Engine.AtlasIsAvailable())
		Engine.RestartInAtlas();
	else
		messageBox(400, 200, translate("The scenario editor is not available or failed to load. See the game logs for additional information."), translate("Error"), 2);
}

function getLobbyDisabledByBuild()
{
	return translate("Launch the multiplayer lobby. \\[DISABLED BY BUILD]");
}

function getTechnicalDetails()
{
	return translate("Technical Details");
}

function getManual()
{
	return translate("Manual");
}

