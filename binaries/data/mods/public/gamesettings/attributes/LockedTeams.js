GameSettings.prototype.Attributes.LockedTeams = class LockedTeams extends GameSetting
{
	init()
	{
		this.enabled = false;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
		this.settings.rating.watch(() => this.onRatingChange(), ["enabled"]);
		this.onRatingChange();
	}

	toInitAttributes(attribs)
	{
		attribs.settings.LockTeams = this.enabled;
	}

	fromInitAttributes(attribs)
	{
		this.enabled = !!this.getLegacySetting(attribs, "LockTeams");
	}

	onMapChange()
	{
		if (this.settings.map.type != "scenario")
			return;
		this.setEnabled(!!this.getMapSetting("LockTeams"));
	}

	onRatingChange()
	{
		if (this.settings.rating.enabled)
		{
			this.available = false;
			this.setEnabled(true);
		}
		else
			this.available = true;
	}

	setEnabled(enabled)
	{
		this.enabled = enabled;
	}
};
