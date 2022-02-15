/**
 * This doesn't have a GUI setting.
 */
GameSettings.prototype.Attributes.CircularMap = class CircularMap extends GameSetting
{
	init()
	{
		this.value = false;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		attribs.settings.CircularMap = this.value;
	}

	fromInitAttributes(attribs)
	{
		this.value = !!this.getLegacySetting(attribs, "CircularMap");
	}

	onMapChange()
	{
		this.value = this.getMapSetting("CircularMap") || false;
	}

	setValue(val)
	{
		this.value = val;
	}
};
