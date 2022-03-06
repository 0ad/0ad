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
		this.settings.playerCount.watch(() => this.maybeUpdate(), ["nbPlayers"]);
	}

	toInitAttributes(attribs)
	{
		if (!attribs.settings.PlayerData)
			attribs.settings.PlayerData = [];
		while (attribs.settings.PlayerData.length < this.values.length)
			attribs.settings.PlayerData.push({});
		for (const i in this.values)
			if (this.values[i])
				attribs.settings.PlayerData[i].StartingCamera = this.values[i];
	}

	fromInitAttributes(attribs)
	{
		if (!this.getLegacySetting(attribs, "PlayerData"))
			return;
		const pData = this.getLegacySetting(attribs, "PlayerData");
		for (let i = 0; i < this.values.length; ++i)
			if (pData[i] && pData[i].StartingCamera !== undefined)
			{
				this.values[i] = pData[i].StartingCamera;
				this.trigger("values");
			}
	}

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

	maybeUpdate()
	{
		if (this.values.length === this.settings.playerCount.nbPlayers)
			return;
		this._resize(this.settings.playerCount.nbPlayers);
		this.trigger("values");
	}
};
