GameSettings.prototype.Attributes.PlayerCount = class PlayerCount extends GameSetting
{
	init()
	{
		this.nbPlayers = 1;
		this.settings.map.watch(() => this.onMapTypeChange(), ["type"]);
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (!attribs.settings.PlayerData)
			attribs.settings.PlayerData = [];
		while (attribs.settings.PlayerData.length < this.nbPlayers)
			attribs.settings.PlayerData.push({});
	}

	fromInitAttributes(attribs)
	{
		if (this.settings.map.map !== undefined)
			this.reloadFromLegacy(attribs);
	}

	onMapTypeChange(old)
	{
		if (this.settings.map.type == "random" && old != "random")
			this.nbPlayers = 2;
	}

	onMapChange()
	{
		if (this.settings.map.type == "random")
			return;
		if (!this.settings.map.data || !this.settings.map.data.settings ||
			!this.settings.map.data.settings.PlayerData)
			return;
		this.nbPlayers = this.settings.map.data.settings.PlayerData.length;
	}

	reloadFromLegacy(data)
	{
		if (this.settings.map.type != "random")
		{
			if (this.settings.map.data && this.settings.map.data.settings && this.settings.map.data.settings.PlayerData)
				this.nbPlayers = this.settings.map.data.settings.PlayerData.length;
			return;
		}
		if (!data || !data.settings || data.settings.PlayerData === undefined)
			return;
		this.nbPlayers = data.settings.PlayerData.length;
	}

	/**
	 * @param index - Player Index, 0 is 'player 1' since GAIA isn't there.
	 */
	get(index)
	{
		return this.data[index];
	}

	setNb(nb)
	{
		this.nbPlayers = Math.max(1, Math.min(g_MaxPlayers, nb));
	}
};
