/**
 * TODO: this would probably be better handled by map triggers.
 * This doesn't have a GUI setting.
 */
GameSettings.prototype.Attributes.StartingTechnologies = class StartingTechnologies extends GameSetting
{
	init()
	{
		this.value = undefined;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.value)
			attribs.settings.StartingTechnologies = this.value;
	}

	/**
	 * Exceptionally, this setting has no Deserialize: it's entirely determined by the map
	 */

	onMapChange()
	{
		if (!this.getMapSetting("StartingTechnologies"))
			this.setValue(undefined);
		else
			this.setValue(this.getMapSetting("StartingTechnologies"));
	}

	setValue(val)
	{
		this.value = val;
	}
};
