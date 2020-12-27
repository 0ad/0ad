class MapPreview
{
	constructor(setupWindow)
	{
		this.setupWindow = setupWindow;
		this.gameSettingsControl = setupWindow.controls.gameSettingsControl;
		this.mapCache = setupWindow.controls.mapCache;

		this.mapInfoName = Engine.GetGUIObjectByName("mapInfoName");
		this.mapPreview = Engine.GetGUIObjectByName("mapPreview");
		this.mapPreview.onMouseLeftPress = this.onPress.bind(this); // TODO: Why does onPress not work? CGUI.cpp seems to support it
		this.mapPreview.tooltip = this.Tooltip;

		this.gameSettingsControl.registerMapChangeHandler(this.onMapChange.bind(this));
		this.gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));
	}

	onPress()
	{
		this.setupWindow.pages.MapBrowserPage.openPage();
	}

	onMapChange(mapData)
	{
		let preview = mapData && mapData.settings && mapData.settings.Preview;
		if (!g_GameAttributes.settings.Preview || g_GameAttributes.settings.Preview != preview)
		{
			g_GameAttributes.settings.Preview = preview;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (!g_GameAttributes.map || !g_GameAttributes.mapType)
			return;

		this.mapInfoName.caption = this.mapCache.translateMapName(
			this.mapCache.getTranslatableMapName(g_GameAttributes.mapType, g_GameAttributes.map));

		this.mapPreview.sprite =
			this.mapCache.getMapPreview(g_GameAttributes.mapType, g_GameAttributes.map, g_GameAttributes);
	}
}

MapPreview.prototype.Tooltip =
	translate("Click to view the list of available maps.");
