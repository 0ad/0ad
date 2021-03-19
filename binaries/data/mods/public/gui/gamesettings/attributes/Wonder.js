GameSettings.prototype.Attributes.Wonder = class Wonder extends GameSetting
{
	init()
	{
		this.available = false;
		this.duration = 0;
		this.settings.victoryConditions.watch(() => this.maybeUpdate(), ["active"]);
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.available)
			attribs.settings.WonderDuration = this.duration;
	}

	fromInitAttributes(attribs)
	{
		if (this.getLegacySetting(attribs, "WonderDuration") !== undefined)
			this.setDuration(+this.getLegacySetting(attribs, "WonderDuration"));
	}

	onMapChange()
	{
		if (this.settings.map.type != "scenario")
			return;
		this.setDuration(+this.getMapSetting("WonderDuration") || 0);
	}

	setDuration(duration)
	{
		this.available = this.settings.victoryConditions.active.has("wonder");
		this.duration = Math.round(duration);
	}

	maybeUpdate()
	{
		this.setDuration(this.duration);
	}
};
