GameSettings.prototype.Attributes.Landscape = class Landscape extends GameSetting
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
			attribs.settings.Landscape = this.value;
	}

	fromInitAttributes(attribs)
	{
		if (!this.getLegacySetting(attribs, "Landscape"))
			this.setValue(undefined);
		else
			this.setValue(this.getLegacySetting(attribs, "Landscape"));
	}

	onMapChange()
	{
		if (!this.getMapSetting("Landscapes"))
		{
			this.value = undefined;
			this.data = undefined;
			return;
		}
		// TODO: validation
		this.data = this.getMapSetting("Landscapes");
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

	getPreviewFilename()
	{
		if (!this.value)
			return undefined;
		for (let group of this.data)
			for (let item of group.Items)
				if (item.Id == this.value)
					return item.Preview;
		return undefined;
	}

	pickRandomItems()
	{
		// If the map is random, we need to wait until it is selected.
		if (this.settings.map.map === "random")
			return true;

		if (!this.value || !this.value.startsWith("random"))
			return false;

		let items = [];
		if (this.value.indexOf("_") !== -1)
		{
			let subgroup = this.data.find(x => x.Id == this.value);
			items = subgroup.Items.map(x => x.Id);
		}
		else
			items = this.data.map(x => x.Items.map(item => item.Id)).flat();

		this.value = pickRandom(items);
		return true;
	}
};
