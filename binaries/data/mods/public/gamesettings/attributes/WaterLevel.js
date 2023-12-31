GameSettings.prototype.Attributes.WaterLevel = class extends GameSetting
{
	init()
	{
		this.settings.map.watch(this.onMapChange.bind(this), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.value)
			attribs.settings.waterLevel = this.value;
	}

	fromInitAttributes(attribs)
	{
		this.value = this.getLegacySetting(attribs, "waterLevel") ?? this.value;
	}

	onMapChange()
	{
		this.options = this.getMapSetting("WaterLevels");
		this.value = this.options?.find(x => x.Default).Name;
	}
};
