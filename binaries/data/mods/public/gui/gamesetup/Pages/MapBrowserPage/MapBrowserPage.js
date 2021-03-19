SetupWindowPages.MapBrowserPage = class extends MapBrowser
{
	constructor(setupWindow)
	{
		super(setupWindow.controls.mapCache, setupWindow.controls.mapFilters, setupWindow);
		this.mapBrowserPage.hidden = true;

		this.gameSettingsControl = setupWindow.controls.gameSettingsControl;
	}

	onSubmitMapSelection(map, type, filter)
	{
		if (!g_IsController)
			return;

		if (type)
			g_GameSettings.map.setType(type);

		if (filter)
			this.gameSettingsControl.guiData.mapFilter.filter = filter;

		if (map)
			g_GameSettings.map.selectMap(map);

		this.gameSettingsControl.setNetworkInitAttributes();
	}

	openPage()
	{
		super.openPage();

		this.controls.MapFiltering.select(
			this.gameSettingsControl.guiData.mapFilter.filter,
			g_GameSettings.map.type || g_MapTypes.Name[g_MapTypes.Default]
		);
		if (g_GameSettings.map.map)
			this.gridBrowser.select(g_GameSettings.map.map);

		this.mapBrowserPage.hidden = false;
	}

	closePage()
	{
		super.closePage();

		this.mapBrowserPage.hidden = true;
	}
};
