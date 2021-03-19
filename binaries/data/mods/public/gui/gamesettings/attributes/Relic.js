GameSettings.prototype.Attributes.Relic = class Relic extends GameSetting
{
	init()
	{
		this.available = false;
		this.count = 0;
		this.duration = 0;
		this.settings.victoryConditions.watch(() => this.maybeUpdate(), ["active"]);
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		// For consistency, only save this if the victory condition is active.
		if (this.available)
		{
			attribs.settings.RelicCount = this.count;
			attribs.settings.RelicDuration = this.duration;
		}
	}

	fromInitAttributes(attribs)
	{
		if (!!this.getLegacySetting(attribs, "RelicCount"))
			this.setCount(this.getLegacySetting(attribs, "RelicCount"));
		if (!!this.getLegacySetting(attribs, "RelicDuration"))
			this.setDuration(this.getLegacySetting(attribs, "RelicDuration"));
	}

	onMapChange()
	{
		if (this.settings.map.type != "scenario")
			return;
		// TODO: probably should sync the victory condition.
		if (!this.getMapSetting("RelicCount"))
			this.available = false;
		else
			this._set(+this.getMapSetting("RelicCount"), +this.getMapSetting("RelicDuration"));
	}

	_set(count, duration)
	{
		this.available = this.settings.victoryConditions.active.has("capture_the_relic");
		this.count = Math.max(1, count);
		this.duration = duration;
	}

	setCount(val)
	{
		this._set(Math.round(val), this.duration);
	}

	setDuration(val)
	{
		this._set(this.count, Math.round(val));
	}

	maybeUpdate()
	{
		this._set(this.count, this.duration);
	}
};
