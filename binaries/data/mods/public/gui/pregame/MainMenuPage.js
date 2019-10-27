/**
 * This is the handler that coordinates all other handlers on this GUI page.
 */
class MainMenuPage
{
	constructor(data, hotloadData, mainMenuItems, backgroundLayerData, projectInformation, communityButtons)
	{
		this.backgroundHandler = new BackgroundHandler(pickRandom(backgroundLayerData));
		this.menuHandler = new MainMenuItemHandler(mainMenuItems);
		this.splashScreenHandler = new SplashScreenHandler(data, hotloadData && hotloadData.splashScreenHandler);

		new MusicHandler();
		new ProjectInformationHandler(projectInformation);
		new CommunityButtonHandler(communityButtons);
	}

	getHotloadData()
	{
		return {
			"splashScreenHandler": this.splashScreenHandler.getHotloadData()
		};
	}
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
	constructor(communityButtons)
	{
		let buttons = Engine.GetGUIObjectByName("communityButtons").children;

		communityButtons.forEach((buttonInfo, i) => {
			let button = buttons[i];
			button.hidden = false;
			for (let propertyName in buttonInfo)
				button[propertyName] = buttonInfo[propertyName];
		});

		if (buttons.length < communityButtons.length)
			error("GUI page has space for " + buttons.length + " community buttons, but " + menuItems.length + " items are provided!");
	}
}
