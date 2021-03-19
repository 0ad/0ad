/**
 * TODO: this would probably be better handled by map triggers.
 * This doesn't have a GUI setting.
 */
GameSettings.prototype.Attributes.DisabledTemplates = class DisabledTemplates extends GameSetting
{
	init()
	{
		this.value = undefined;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.value)
			attribs.settings.DisabledTemplates = this.value;
	}

	/**
	 * Exceptionally, this setting has no Deserialize: it's entirely determined by the map
	 */

	onMapChange()
	{
		if (!this.getMapSetting("DisabledTemplates"))
			this.setValue(undefined);
		else
			this.setValue(this.getMapSetting("DisabledTemplates"));
	}

	setValue(val)
	{
		this.value = val;
	}
};
