class MapPreview
{
	constructor(gameSettingsControl, mapCache)
	{
		this.mapCache = mapCache;

		this.mapInfoName = Engine.GetGUIObjectByName("mapInfoName");
		this.mapPreview = Engine.GetGUIObjectByName("mapPreview");

		gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));
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
