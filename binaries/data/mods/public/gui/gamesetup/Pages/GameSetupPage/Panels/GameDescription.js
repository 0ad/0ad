class GameDescription
{
	constructor(setupWindow, gameSettingTabs)
	{
		this.mapCache = setupWindow.controls.mapCache;

		this.gameDescriptionFrame = Engine.GetGUIObjectByName("gameDescriptionFrame");
		this.gameDescription = Engine.GetGUIObjectByName("gameDescription");

		setupWindow.controls.gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));
		gameSettingTabs.registerTabsResizeHandler(this.onTabsResize.bind(this));
	}

	onTabsResize(settingsTabButtonsFrame)
	{
		let size = this.gameDescriptionFrame.size;
		size.top = settingsTabButtonsFrame.size.bottom + this.Margin;
		this.gameDescriptionFrame.size = size;
	}

	onGameAttributesBatchChange()
	{
		this.gameDescription.caption = getGameDescription(this.mapCache);
	}
}

GameDescription.prototype.Margin = 3;
