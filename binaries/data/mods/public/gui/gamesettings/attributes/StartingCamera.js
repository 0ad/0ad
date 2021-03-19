/**
 * For compatibility reasons, this loads the per-player StartingCamera from the map data
 * In general, this is probably better handled by map triggers or the default camera placement.
 * This doesn't have a GUI setting.
 */
GameSettings.prototype.Attributes.StartingCamera = class StartingCamera extends GameSetting
{
	init()
	{
		this.values = [];
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (!attribs.settings.PlayerData)
			attribs.settings.PlayerData = [];
		while (attribs.settings.PlayerData.length < this.values.length)
			attribs.settings.PlayerData.push({});
		for (let i in this.values)
			if (this.values[i])
				attribs.settings.PlayerData[i].StartingCamera = this.values[i];
	}

	/**
	 * Exceptionally, this setting has no Deserialize: it's entirely determined by the map
	 */

	_resize(nb)
	{
		while (this.values.length > nb)
			this.values.pop();
		while (this.values.length < nb)
			this.values.push(undefined);
	}

	onMapChange()
	{
		let pData = this.getMapSetting("PlayerData");
		this._resize(pData?.length || 0);
		for (let i in pData)
			this.values[i] = pData?.[i]?.StartingCamera;
	}
};
