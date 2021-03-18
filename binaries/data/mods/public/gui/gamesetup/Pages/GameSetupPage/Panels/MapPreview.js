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

		g_GameSettings.map.watch(() => this.renderName(), ["map"]);
		g_GameSettings.mapPreview.watch(() => this.renderPreview(), ["value"]);
	}

	onPress()
	{
		this.setupWindow.pages.MapBrowserPage.openPage();
	}

	renderName()
	{
		if (!g_GameSettings.map.map)
		{
			this.mapInfoName.caption = translate("No selected map");
			return;
		}

		this.mapInfoName.caption = this.mapCache.translateMapName(
			this.mapCache.getTranslatableMapName(g_GameSettings.map.type, g_GameSettings.map.map));
	}

	renderPreview()
	{
		if (!g_GameSettings.mapPreview.value)
		{
			this.mapPreview.sprite = this.mapCache.getMapPreview();
			return;
		}
		this.mapPreview.sprite = g_GameSettings.mapPreview.value;
	}
}

MapPreview.prototype.Tooltip =
	translate("Click to view the list of available maps.");
