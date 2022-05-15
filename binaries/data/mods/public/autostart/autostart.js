class AutoStart
{
	constructor(initData)
	{
		this.settings = new GameSettings().init();
		this.settings.fromInitAttributes(initData.attribs);

		this.playerAssignments = initData.playerAssignments;

		this.settings.launchGame(this.playerAssignments, initData.storeReplay);

		this.onLaunch();
	}

	onTick()
	{
	}

	/**
	 * In the visual autostart path, we need to show the loading screen.
	 */
	onLaunch()
	{
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": this.settings.finalizedAttributes,
			"playerAssignments": this.playerAssignments
		});
	}
}
