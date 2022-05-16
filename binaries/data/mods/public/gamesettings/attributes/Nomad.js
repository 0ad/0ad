GameSettings.prototype.Attributes.Nomad = class Nomad extends GameSetting
{
	init()
	{
		this.enabled = false;
	}

	toInitAttributes(attribs)
	{
		if (this.settings.map.type == "random")
			attribs.settings.Nomad = this.enabled;
	}

	fromInitAttributes(attribs)
	{
		this.setEnabled(!!this.getLegacySetting(attribs, "Nomad"));
	}

	setEnabled(enabled)
	{
		this.enabled = enabled;
	}
};
