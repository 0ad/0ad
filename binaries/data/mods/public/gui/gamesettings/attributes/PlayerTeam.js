/**
 * Stores team settings for all players.
 */
GameSettings.prototype.Attributes.PlayerTeam = class PlayerTeam extends GameSetting
{
	init()
	{
		// NB: watchers aren't auto-triggered when modifying array elements.
		this.values = [];
		this.locked = [];
		this.settings.playerCount.watch(() => this.maybeUpdate(), ["nbPlayers"]);
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (!attribs.settings.PlayerData)
			attribs.settings.PlayerData = [];
		while (attribs.settings.PlayerData.length < this.values.length)
			attribs.settings.PlayerData.push({});
		for (let i in this.values)
			if (this.values[i] !== undefined)
				attribs.settings.PlayerData[i].Team = this.values[i];
	}

	fromInitAttributes(attribs)
	{
		if (!this.getLegacySetting(attribs, "PlayerData"))
			return;
		const pData = this.getLegacySetting(attribs, "PlayerData");
		for (let i = 0; i < this.values.length; ++i)
			if (pData[i] && pData[i].Team !== undefined)
				this.setValue(i, pData[i].Team);
	}

	_resize(nb)
	{
		while (this.values.length > nb)
		{
			this.values.pop();
			this.locked.pop();
		}
		while (this.values.length < nb)
		{
			// -1 is None
			this.values.push(-1);
			this.locked.push(false);
		}
	}

	onMapChange()
	{
		this.locked = this.locked.map(x => this.settings.map.type === "scenario");
		this.trigger("locked");
		if (this.settings.map.type !== "scenario")
			return;
		const pData = this.getMapSetting("PlayerData");
		for (const p in pData)
			this._set(+p, pData[p].Team === undefined ? -1 : pData[p].Team);
		this.trigger("values");
	}

	maybeUpdate()
	{
		this._resize(this.settings.playerCount.nbPlayers);
		this.values.forEach((c, i) => this._set(i, c));
		this.trigger("values");
	}

	_set(playerIndex, value)
	{
		this.values[playerIndex] = value;
	}

	setValue(playerIndex, val)
	{
		this._set(playerIndex, val);
		this.trigger("values");
	}
};
