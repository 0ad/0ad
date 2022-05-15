GameSettings.prototype.Attributes.DisableTreasures = class DisableTreasures extends GameSetting
{
	init()
	{
		this.enabled = false;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		attribs.settings.DisableTreasures = this.enabled;
	}

	fromInitAttributes(attribs)
	{
		this.enabled = !!this.getLegacySetting(attribs, "DisableTreasures");
	}

	onMapChange()
	{
		if (this.settings.map.type != "scenario")
			return;
		this.setEnabled(!!this.getMapSetting("DisableTreasures"));
	}

	setEnabled(enabled)
	{
		this.enabled = enabled;
	}
};
