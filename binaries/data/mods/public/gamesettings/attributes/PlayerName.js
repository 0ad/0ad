/**
 * Stores in-game names for all players.
 *
 * NB: the regular gamesetup has a particular handling of this setting.
 * The names are loaded from the map, but the GUI also show playernames.
 * Force these at the start of the match.
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

	fromInitAttributes(attribs)
	{
		if (!this.getLegacySetting(attribs, "PlayerData"))
			return;
		const pData = this.getLegacySetting(attribs, "PlayerData");
		for (let i = 0; i < this.values.length; ++i)
			if (pData[i] && pData[i].Name !== undefined)
			{
				this.values[i] = pData[i].Name;
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
	 * Pick AI names.
	 */
	pickRandomItems()
	{
		const AIPlayerNamesList = [];
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

			const names = this.settings.civData[civ].AINames;
			const remainingNames = names.filter(name => !AIPlayerNamesList.includes(name));
			const chosenName = pickRandom(remainingNames.length ? remainingNames : names);
			
			// Avoid translating AI names if the game is networked, so all players see and refer to
			// English names instead of names in the language of the host.
			const translatedCountLabel = this.settings.isNetworked ? this.CountLabel : translate(this.CountLabel);
			const translatedChosenName = this.settings.isNetworked ? chosenName : translate(chosenName);

			const duplicateNameCount = AIPlayerNamesList.reduce((count, name) => {
				if (name == chosenName)
					count++;
				return count;
			}, 0);
			
			AIPlayerNamesList.push(chosenName);

			this.values[i] = !duplicateNameCount ? translatedChosenName :
				sprintf(translatedCountLabel, {
					"playerName": translatedChosenName,
					"nameCount": duplicateNameCount + 1
				});
		}
		if (picked)
			this.trigger("values");
		return picked;
	}

	onFinalizeAttributes(attribs, playerAssignments)
	{
		// Replace client player names with the real players.
		for (const guid in playerAssignments)
			if (playerAssignments[guid].player !== -1)
				attribs.settings.PlayerData[playerAssignments[guid].player -1].Name = playerAssignments[guid].name;
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

/** Translation: This is a template (sprintf format specifier) for the name of
 * an AI-controlled player and a unique number for each of the players with
 * that same name. Example: Perseus (2)
 */
GameSettings.prototype.Attributes.PlayerName.prototype.CountLabel = markForTranslation("%(playerName)s (%(nameCount)i)");
