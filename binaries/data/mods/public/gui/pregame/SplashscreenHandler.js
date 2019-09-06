class SplashScreenHandler
{
	constructor(initData, hotloadData)
	{
		this.showSplashScreen = hotloadData ? hotloadData.showSplashScreen : initData && initData.isStartup;
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
	onTick()
	{
		if (this.showSplashScreen)
			this.openPage();
	}

	openPage()
	{
		this.showSplashScreen = false;

		if (Engine.ConfigDB_GetValue("user", "gui.splashscreen.enable") === "true" ||
		    Engine.ConfigDB_GetValue("user", "gui.splashscreen.version") < Engine.GetFileMTime("gui/splashscreen/splashscreen.txt"))
			Engine.PushGuiPage("page_splashscreen.xml", {}, this.showRenderPathMessage);
		else
			this.showRenderPathMessage();
	}

	showRenderPathMessage()
	{
		// Warn about removing fixed render path
		if (Engine.Renderer_GetRenderPath() != "fixed")
			return;

		messageBox(
			600, 300,
			"[font=\"sans-bold-16\"]" +
			sprintf(translate("%(warning)s You appear to be using non-shader (fixed function) graphics. This option will be removed in a future 0 A.D. release, to allow for more advanced graphics features. We advise upgrading your graphics card to a more recent, shader-compatible model."), {
				"warning": coloredText("Warning:", "200 20 20")
			}) +
			"\n\n" +
			// Translation: This is the second paragraph of a warning. The
			// warning explains that the user is using “non-shader“ graphics,
			// and that in the future this will not be supported by the game, so
			// the user will need a better graphics card.
			translate("Please press \"Read More\" for more information or \"OK\" to continue."),
			translate("WARNING!"),
			[translate("OK"), translate("Read More")],
			[
				null,
				() => {
					Engine.OpenURL("https://www.wildfiregames.com/forum/index.php?showtopic=16734");
				}
			]);
	}
}
