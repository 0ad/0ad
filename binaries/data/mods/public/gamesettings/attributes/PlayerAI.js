/**
 * Stores AI settings for all players.
 * Note that tby default, this does not assign AI
 * unless an AI bot is explicitly specified.
 * This is because:
 *  - the regular GameSetup does that on its own
 *  - this makes campaign/autostart scenarios easier to handle.
 *  - cleans the code here.
 */
GameSettings.prototype.Attributes.PlayerAI = class PlayerAI extends GameSetting
{
	init()
	{
		// NB: watchers aren't auto-triggered when modifying array elements.
		this.values = [];
		this.settings.playerCount.watch(() => this.maybeUpdate(), ["nbPlayers"]);
	}

	toInitAttributes(attribs)
	{
		if (!attribs.settings.PlayerData)
			attribs.settings.PlayerData = [];
		while (attribs.settings.PlayerData.length < this.values.length)
			attribs.settings.PlayerData.push({});
		for (let i = 0; i < this.values.length; ++i)
			if (this.values[i])
			{
				attribs.settings.PlayerData[i].AI = this.values[i].bot;
				attribs.settings.PlayerData[i].AIDiff = this.values[i].difficulty;
				attribs.settings.PlayerData[i].AIBehavior = this.values[i].behavior;
			}
			else
				attribs.settings.PlayerData[i].AI = false;
	}

	fromInitAttributes(attribs)
	{
		if (!this.getLegacySetting(attribs, "PlayerData"))
			return;
		const pData = this.getLegacySetting(attribs, "PlayerData");
		for (let i = 0; i < this.values.length; ++i)
		{
			// Also covers the "" case.
			if (!pData[i] || !pData[i].AI)
			{
				this.set(+i, undefined);
				continue;
			}
			this.set(+i, {
				"bot": pData[i].AI,
				"difficulty": pData[i].AIDiff ?? +Engine.ConfigDB_GetValue("user", "gui.gamesetup.aidifficulty"),
				"behavior": pData[i].AIBehavior || Engine.ConfigDB_GetValue("user", "gui.gamesetup.aibehavior"),
			});
		}
	}

	_resize(nb)
	{
		while (this.values.length > nb)
			this.values.pop();
		while (this.values.length < nb)
			this.values.push(undefined);
	}

	maybeUpdate()
	{
		if (this.values.length === this.settings.playerCount.nbPlayers)
			return;
		this._resize(this.settings.playerCount.nbPlayers);
		this.trigger("values");
	}

	swap(sourceIndex, targetIndex)
	{
		[this.values[sourceIndex], this.values[targetIndex]] = [this.values[targetIndex], this.values[sourceIndex]];
		this.trigger("values");
	}

	set(playerIndex, botSettings)
	{
		this.values[playerIndex] = botSettings;
		this.trigger("values");
	}

	setAI(playerIndex, ai)
	{
		let old = this.values[playerIndex] ? this.values[playerIndex].bot : undefined;
		if (!ai)
			this.values[playerIndex] = undefined;
		else
			this.values[playerIndex].bot = ai;
		if (old !== (this.values[playerIndex] ? this.values[playerIndex].bot : undefined))
			this.trigger("values");
	}

	setBehavior(playerIndex, value)
	{
		if (!this.values[playerIndex])
			return;
		this.values[playerIndex].behavior = value;
		this.trigger("values");
	}

	setDifficulty(playerIndex, value)
	{
		if (!this.values[playerIndex])
			return;
		this.values[playerIndex].difficulty = value;
		this.trigger("values");
	}

	get(playerIndex)
	{
		return this.values[playerIndex];
	}

	describe(playerIndex)
	{
		if (!this.values[playerIndex])
			return "";
		return translateAISettings({
			"AI": this.values[playerIndex].bot,
			"AIDiff": this.values[playerIndex].difficulty,
			"AIBehavior": this.values[playerIndex].behavior,
		});
	}
};
