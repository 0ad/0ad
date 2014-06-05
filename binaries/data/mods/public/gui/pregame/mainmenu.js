var userReportEnabledText; // contains the original version with "$status" placeholder
var currentSubmenuType; // contains submenu type
const MARGIN = 4; // menu border size
const background = "hellenes1"; // Background type. Currently: 'hellenes1', 'persians1'.

var g_ShowSplashScreens;

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
}

function getHotloadData()
{
	return { "showSplashScreens": g_ShowSplashScreens };
}

var t0 = new Date;
function scrollBackgrounds(background)
{
	if (background == "hellenes1")
	{
		var layer1 = Engine.GetGUIObjectByName("backgroundHele1-1");
		var layer2 = Engine.GetGUIObjectByName("backgroundHele1-2");
		var layer3 = Engine.GetGUIObjectByName("backgroundHele1-3");
		
		layer1.hidden = false;
		layer2.hidden = false;
		layer3.hidden = false;
	 
		var screen = layer1.parent.getComputedSize();
		var h = screen.bottom - screen.top; // height of screen
		var w = h*16/9; // width of background image
	
		// Offset the layers by oscillating amounts
		var t = (t0 - new Date) / 700;
		var speed = 1/20;
		var off1 = 0.02 * w * (1+Math.cos(t*speed));
		var off2 = 0.12 * w * (1+Math.cos(t*speed)) - h*6/9;
		var off3 = 0.16 * w * (1+Math.cos(t*speed));
	
		var left = screen.right - w * (1 + Math.ceil(screen.right / w));
		layer1.size = new GUISize(left + off1, screen.top, screen.right + off1, screen.bottom);
		layer2.size = new GUISize(screen.right/2 - h + off2, screen.top, screen.right/2 + h + off2, screen.bottom);
		layer3.size = new GUISize(screen.right - h + off3, screen.top, screen.right + off3, screen.bottom);
	}
	
	if (background == "persians1")
	{
		var layer1 = Engine.GetGUIObjectByName("backgroundPers1-1");
		var layer2 = Engine.GetGUIObjectByName("backgroundPers1-2");
		var layer3 = Engine.GetGUIObjectByName("backgroundPers1-3");
		var layer4 = Engine.GetGUIObjectByName("backgroundPers1-4");
		
		layer1.hidden = false;
		layer2.hidden = false;
		layer3.hidden = false;
		layer4.hidden = false;
		
		var screen = layer1.parent.getComputedSize();
		var h = screen.bottom - screen.top; // height of screen
		var screenWidth = screen.right - screen.left;
		var w = h*16/9;
		
		var t = (t0 - new Date) / 1000;
		var speed = 1/20;
		var off1 = 0.01 * w * (Math.cos(t*speed));
		var off2 = 0.03 * w * (Math.cos(t*speed));
		var off3 =  0.07 * w * (1+Math.cos(t*speed)) + 0.5 * screenWidth - h*1.1;
		var off4 =  0.16 * w * (1+Math.cos(t*speed)) - h*6/9;
		
		var left = screen.right - w * (1 + Math.ceil(screen.right / w)) - 0.5 * screenWidth + h;
		layer1.size = new GUISize(left + off1, screen.top, screen.right + off1 + h, screen.bottom);
		layer2.size = new GUISize(left + off2, screen.top, screen.right + off2 + h, screen.bottom);
		layer3.size = new GUISize(screen.left + off3, screen.top, screen.left + 2 * h + off3, screen.bottom);
		layer4.size = new GUISize(screen.left + off4, screen.top, screen.left + 2 * h + off4, screen.bottom);
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
	scrollBackgrounds(background);

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
	return translate("Launch the multiplayer lobby. [DISABLED BY BUILD]");
}

function getTechnicalDetails()
{
	return translate("Technical Details");
}

function getManual()
{
	return translate("Manual");
}

