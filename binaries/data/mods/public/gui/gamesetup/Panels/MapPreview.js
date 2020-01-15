class MapPreview
{
	constructor(gameSettingsControl, mapCache)
	{
		this.gameSettingsControl = gameSettingsControl;
		this.mapCache = mapCache;

		this.mapInfoName = Engine.GetGUIObjectByName("mapInfoName");
		this.mapPreview = Engine.GetGUIObjectByName("mapPreview");

		gameSettingsControl.registerMapChangeHandler(this.onMapChange.bind(this));
		gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));
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
