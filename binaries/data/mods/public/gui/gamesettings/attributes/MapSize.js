GameSettings.prototype.Attributes.MapSize = class MapSize extends GameSetting
{
	init()
	{
		this.defaultValue = this.getDefaultValue("MapSizes", "Tiles") || 256;
		this.setSize(this.defaultValue);
		this.settings.map.watch(() => this.onTypeChange(), ["type"]);
	}

	toInitAttributes(attribs)
	{
		if (this.settings.map.type === "random")
			attribs.settings.Size = this.size;
	}

	fromInitAttributes(attribs)
	{
		if (!!this.getLegacySetting(attribs, "Size"))
			this.setSize(this.getLegacySetting(attribs, "Size"));
	}

	setSize(size)
	{
		this.available = this.settings.map.type === "random";
		this.size = size;
	}

	onTypeChange(old)
	{
		if (this.settings.map.type === "random" && old !== "random")
			this.setSize(this.defaultValue);
		else if (this.settings.map.type !== "random")
			this.available = false;
	}
};
