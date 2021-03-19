/**
 * This doesn't have a GUI setting.
 */
GameSettings.prototype.Attributes.CircularMap = class CircularMap extends GameSetting
{
	init()
	{
		this.value = undefined;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.value)
			attribs.settings.CircularMap = this.value;
	}

	/**
	 * Exceptionally, this setting has no Deserialize: it's entirely determined by the map
	 */

	onMapChange()
	{
		this.value = this.getMapSetting("CircularMap") || false;
	}

	setValue(val)
	{
		this.value = val;
	}
};
