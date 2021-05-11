GameSettings.prototype.Attributes.Rating = class Rating extends GameSetting
{
	init()
	{
		this.hasXmppClient = Engine.HasXmppClient();
		this.settings.playerCount.watch(() => this.maybeUpdate(), ["nbPlayers"]);
		this.settings.cheats.watch(() => this.maybeUpdate(), ["enabled"]);
		this.maybeUpdate();
	}

	toInitAttributes(attribs)
	{
		if (this.available)
			attribs.settings.RatingEnabled = this.enabled;
	}

	fromInitAttributes(attribs)
	{
		if (this.getLegacySetting(attribs, "RatingEnabled") !== undefined)
		{
			this.available = this.hasXmppClient && this.settings.playerCount.nbPlayers === 2;
			this.enabled = this.available && !!this.getLegacySetting(attribs, "RatingEnabled");
		}
	}

	setEnabled(enabled)
	{
		this.enabled = this.available && enabled;
	}

	maybeUpdate()
	{
		// This setting is activated by default if it's possible.
		this.available = this.hasXmppClient &&
			this.settings.playerCount.nbPlayers === 2 &&
			!this.settings.cheats.enabled;
		this.enabled = this.available;
	}
};
