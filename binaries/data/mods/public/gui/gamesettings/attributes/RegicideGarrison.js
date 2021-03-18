GameSettings.prototype.Attributes.RegicideGarrison = class RegicideGarrison extends GameSetting
{
	init()
	{
		this.setEnabled(false);
		this.settings.victoryConditions.watch(() => this.maybeUpdate(), ["active"]);
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		attribs.settings.RegicideGarrison = this.enabled;
	}

	fromInitAttributes(attribs)
	{
		this.enabled = !!this.getLegacySetting(attribs, "RegicideGarrison");
	}

	onMapChange()
	{
		if (this.settings.map.type != "scenario")
			return;
		this.setEnabled(!!this.getMapSetting("RegicideGarrison"));
	}

	setEnabled(enabled)
	{
		this.available = this.settings.victoryConditions.active.has("regicide");
		this.enabled = (enabled && this.available);
	}

	maybeUpdate()
	{
		this.setEnabled(this.enabled);
	}
};
