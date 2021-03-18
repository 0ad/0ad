GameSettings.prototype.Attributes.Cheats = class Cheats extends GameSetting
{
	init()
	{
		this.enabled = false;
		this.settings.rating.watch(() => this.maybeUpdate(), ["enabled"]);
	}

	toInitAttributes(attribs)
	{
		attribs.settings.CheatsEnabled = this.enabled;
	}

	fromInitAttributes(attribs)
	{
		this.enabled = !!this.getLegacySetting(attribs, "CheatsEnabled");
	}

	_set(enabled)
	{
		this.enabled = (enabled && !this.settings.rating.enabled);
	}

	setEnabled(enabled)
	{
		this._set(enabled);
	}

	maybeUpdate()
	{
		this._set(this.enabled);
	}
};
