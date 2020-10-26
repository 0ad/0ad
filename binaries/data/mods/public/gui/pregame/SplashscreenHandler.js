class SplashScreenHandler
{
	constructor(initData, hotloadData)
	{
		this.showSplashScreen = hotloadData ? hotloadData.showSplashScreen : initData && initData.isStartup;

		this.mainMenuPage = Engine.GetGUIObjectByName("mainMenuPage");
		this.mainMenuPage.onTick = this.onFirstTick.bind(this);
	}

	getHotloadData()
	{
		// Only show splash screen(s) once at startup, but not again after hotloading
		return {
			"showSplashScreen": this.showSplashScreen
		};
	}

	// Don't call this from the init function in order to not crash when opening the new page on init on hotloading
	// and not possibly crash when opening the new page on init and throwing a JS error.
	onFirstTick()
	{
		if (this.showSplashScreen)
			this.openPage();

		delete this.mainMenuPage.onTick;
	}

	openPage()
	{
		this.showSplashScreen = false;

		if (Engine.ConfigDB_GetValue("user", "gui.splashscreen.enable") === "true" ||
		    Engine.ConfigDB_GetValue("user", "gui.splashscreen.version") < Engine.GetFileMTime("gui/splashscreen/splashscreen.txt"))
			Engine.PushGuiPage("page_splashscreen.xml", {});
	}
}
