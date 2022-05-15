GameSettings.prototype.Attributes.TriggerDifficulty = class TriggerDifficulty extends GameSetting
{
	init()
	{
		this.difficulties = loadSettingValuesFile("trigger_difficulties.json");
		this.available = undefined;
		this.value = undefined;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.available)
			attribs.settings.TriggerDifficulty = this.value;
	}

	fromInitAttributes(attribs)
	{
		if (this.getLegacySetting(attribs, "TriggerDifficulty") !== undefined)
			this.setValue(this.getLegacySetting(attribs, "TriggerDifficulty"));
	}

	getAvailableSettings()
	{
		return this.difficulties.filter(x => this.available.indexOf(x.Name) !== -1);
	}

	onMapChange()
	{
		if (!this.getMapSetting("SupportedTriggerDifficulties"))
		{
			this.value = undefined;
			this.available = undefined;
			return;
		}
		// TODO: should probably validate that they fit one of the known schemes.
		this.available = this.getMapSetting("SupportedTriggerDifficulties").Values;
		this.value = this.difficulties.find(x => x.Default && this.available.indexOf(x.Name) !== -1).Difficulty;
	}

	setValue(val)
	{
		this.value = val;
	}

	getData()
	{
		if (!this.value)
			return undefined;
		return this.difficulties[this.value - 1];
	}
};
