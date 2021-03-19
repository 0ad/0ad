GameSettings.prototype.Attributes.SeaLevelRise = class SeaLevelRise extends GameSetting
{
	init()
	{
		this.min = undefined;
		this.max = undefined;
		this.value = undefined;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.value)
			attribs.settings.SeaLevelRiseTime = this.value;
	}

	fromInitAttributes(attribs)
	{
		if (!!this.getLegacySetting(attribs, "SeaLevelRiseTime"))
			this.setValue(this.getLegacySetting(attribs, "SeaLevelRiseTime"));
	}

	onMapChange()
	{
		if (!this.getMapSetting("SeaLevelRise"))
		{
			this.value = undefined;
			return;
		}
		let mapData = this.settings.map.data;
		this.min = mapData.settings.SeaLevelRise.Min;
		this.max = mapData.settings.SeaLevelRise.Max;
		this.value = mapData.settings.SeaLevelRise.Default;
	}

	setValue(val)
	{
		if (!this.getMapSetting("SeaLevelRise"))
			this.value = undefined;
		else
			this.value = Math.max(this.min, Math.min(this.max, Math.round(val)));
	}
};
