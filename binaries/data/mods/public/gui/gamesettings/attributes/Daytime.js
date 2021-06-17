GameSettings.prototype.Attributes.Daytime = class Daytime extends GameSetting
{
	init()
	{
		this.data = undefined;
		this.value = undefined;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.value)
			attribs.settings.Daytime = this.value;
	}

	fromInitAttributes(attribs)
	{
		if (!this.getLegacySetting(attribs, "Daytime"))
			this.setValue(undefined);
		else
			this.setValue(this.getLegacySetting(attribs, "Daytime"));
	}

	onMapChange()
	{
		let mapData = this.settings.map.data;
		if (!mapData || !mapData.settings || !mapData.settings.Daytime)
		{
			this.value = undefined;
			this.data = undefined;
			return;
		}
		// TODO: validation
		this.data = mapData.settings.Daytime;
		this.value = "random";
	}

	setValue(val)
	{
		// TODO: more validation.
		if (this.data)
			this.value = val || "random";
		else
			this.value = undefined;
	}

	pickRandomItems()
	{
		// If the map is random, we need to wait until it is selected.
		if (this.settings.map.map === "random")
			return true;

		if (this.value !== "random")
			return false;
		this.value = pickRandom(this.data).Id;
		return true;
	}
};
