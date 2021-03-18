GameSettings.prototype.Attributes.Ceasefire = class Ceasefire extends GameSetting
{
	init()
	{
		this.value = 0;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (!this.value)
			return;
		attribs.settings.Ceasefire = this.value;
	}

	fromInitAttributes(attribs)
	{
		if (!this.getLegacySetting(attribs, "Ceasefire"))
			this.value = 0;
		else
			this.value = +this.getLegacySetting(attribs, "Ceasefire");
	}

	onMapChange()
	{
		if (this.settings.map.type != "scenario")
			return;
		if (!this.getMapSetting("Ceasefire"))
			this.value = 0;
		else
			this.value = +this.getMapSetting("Ceasefire");
	}

	setValue(val)
	{
		this.value = Math.round(val);
	}
};
