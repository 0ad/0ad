/**
 * Stores in-game names for all players.
 *
 * NB: the regular gamesetup has a particular handling of this setting.
 * The names are loaded from the map, but the GUI also show playernames
 * and forces them when starting the game.
 * This is therefore just handling map-defined names & AI random bot names.
 */
GameSettings.prototype.Attributes.PlayerName = class PlayerName extends GameSetting
{
	init()
	{
		// NB: watchers aren't auto-triggered when modifying array elements.
		this.values = [];
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
			if (this.values[i])
				attribs.settings.PlayerData[i].Name = this.values[i];
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
		// Reset.
		this._resize(0);
		this.maybeUpdate();
	}

	maybeUpdate()
	{
		this._resize(this.settings.playerCount.nbPlayers);
		this.values.forEach((_, i) => this._set(i));
		this.trigger("values");
	}

	/**
	 * Pick bot names.
	 */
	pickRandomItems()
	{
		let picked = false;
		for (let i in this.values)
		{
			if (!!this.values[i] &&
				this.values[i] !== g_Settings.PlayerDefaults[+i + 1].Name)
				continue;

			let ai = this.settings.playerAI.values[i];
			if (!ai)
				continue;

			let civ = this.settings.playerCiv.values[i];
			if (!civ || civ == "random")
				continue;

			picked = true;
			// Pick one of the available botnames for the chosen civ
			// Determine botnames
			let chosenName = pickRandom(this.settings.civData[civ].AINames);

			// Count how many players use the chosenName
			let usedName = this.values.filter(oName => oName && oName.indexOf(chosenName) !== -1).length;

			this.values[i] =
				usedName ?
					sprintf(this.RomanLabel, {
						"playerName": chosenName,
						"romanNumber": this.RomanNumbers[usedName + 1]
					}) :
					chosenName;
		}
		if (picked)
			this.trigger("values");
		return picked;
	}

	_getMapData(i)
	{
		let data = this.settings.map.data;
		if (!data || !data.settings || !data.settings.PlayerData)
			return undefined;
		if (data.settings.PlayerData.length <= i)
			return undefined;
		return data.settings.PlayerData[i].Name;
	}

	_set(playerIndex)
	{
		this.values[playerIndex] = this._getMapData(playerIndex) || g_Settings && g_Settings.PlayerDefaults[playerIndex + 1].Name || "";
	}
};


GameSettings.prototype.Attributes.PlayerName.prototype.RomanLabel =
	translate("%(playerName)s %(romanNumber)s");

GameSettings.prototype.Attributes.PlayerName.prototype.RomanNumbers =
	[undefined, "I", "II", "III", "IV", "V", "VI", "VII", "VIII"];
