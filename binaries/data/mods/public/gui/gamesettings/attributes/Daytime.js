GameSettings.prototype.Attributes.Daytime = class Daytime extends GameSetting
{
	init()
	{
		this.setDataValueHelper(undefined, undefined);
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
			this.setDataValueHelper(undefined, undefined);
			return;
		}
		// TODO: validation
		this.setDataValueHelper(mapData.settings.Daytime, "random");
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

	/**
	 * Helper function to ensure this.data and this.value
	 * are assigned in the correct order to prevent
	 * crashes in the renderer.
	 * @param {object} data - The day time option data.
	 * @param {string} value - The option's key.
	*/
	setDataValueHelper(data, value)
	{
		this.data = data;
		this.value = value;
	}
};
