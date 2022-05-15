GameSettings.prototype.Attributes.LastManStanding = class LastManStanding extends GameSetting
{
	init()
	{
		this.available = !this.settings.lockedTeams.enabled;
		this.enabled = false;
		this.settings.lockedTeams.watch(() => this.maybeUpdate(), ["enabled"]);
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		attribs.settings.LastManStanding = this.enabled;
	}

	fromInitAttributes(attribs)
	{
		this.enabled = !!this.getLegacySetting(attribs, "LastManStanding");
	}

	onMapChange()
	{
		if (this.settings.map.type != "scenario")
			return;
		this.setEnabled(!!this.getMapSetting("LastManStanding"));
	}

	setEnabled(enabled)
	{
		this.available = !this.settings.lockedTeams.enabled;
		this.enabled = (enabled && this.available);
	}

	maybeUpdate()
	{
		this.setEnabled(this.enabled);
	}
};
