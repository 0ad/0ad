GameSettings.prototype.Attributes.DisableSpies = class DisableSpies extends GameSetting
{
	init()
	{
		this.enabled = false;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		attribs.settings.DisableSpies = this.enabled;
	}

	fromInitAttributes(attribs)
	{
		this.enabled = !!this.getLegacySetting(attribs, "DisableSpies");
	}

	onMapChange()
	{
		if (this.settings.map.type != "scenario")
			return;
		this.setEnabled(!!this.getMapSetting("DisableSpies"));
	}

	setEnabled(enabled)
	{
		this.enabled = enabled;
	}
};
