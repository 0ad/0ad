/**
 * Available backgrounds, added by the files in backgrounds/.
 */
var g_BackgroundLayerData = [];

var g_BackgroundHandler;
var g_MenuHandler;
var g_SplashScreenHandler;

function init(data, hotloadData)
{
	g_MenuHandler = new MainMenuItemHandler(g_MainMenuItems);
	g_BackgroundHandler = new BackgroundHandler(pickRandom(g_BackgroundLayerData));
	g_SplashScreenHandler = new SplashScreenHandler(data, hotloadData && hotloadData.splashScreenHandler);

	new MusicHandler();
	new ProjectInformationHandler(g_ProjectInformation);
	new CommunityButtonHandler();
}

function onTick()
{
	g_MenuHandler.onTick();
	g_BackgroundHandler.onTick();
	g_SplashScreenHandler.onTick();
}

function getHotloadData()
{
	return {
		"splashScreenHandler": g_SplashScreenHandler.getHotloadData()
	};
}

class MusicHandler
{
	constructor()
	{
		initMusic();
		global.music.setState(global.music.states.MENU);
	}
}

class ProjectInformationHandler
{
	constructor(projectInformation)
	{
		for (let objectName in projectInformation)
			for (let propertyName in projectInformation[objectName])
				Engine.GetGUIObjectByName(objectName)[propertyName] = projectInformation[objectName][propertyName];
	}
}

class CommunityButtonHandler
{
	constructor()
	{
		let buttons = Engine.GetGUIObjectByName("communityButtons").children;

		g_CommunityButtons.forEach((buttonInfo, i) => {
			let button = buttons[i];
			button.hidden = false;
			for (let propertyName in buttonInfo)
				button[propertyName] = buttonInfo[propertyName];
		});

		if (buttons.length < g_CommunityButtons.length)
			error("GUI page has space for " + buttons.length + " community buttons, but " + menuItems.length + " items are provided!");
	}
}
