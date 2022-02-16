/**
 * Map name.
 * This is usually just the regular map name, but can be overwritten.
 */
GameSettings.prototype.Attributes.MapName = class MapName extends GameSetting
{
	init()
	{
		this.value = undefined;
		this.settings.map.watch(() => this.updateName(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		attribs.settings.mapName = this.value;
	}

	fromInitAttributes(attribs)
	{
		if (attribs?.settings?.mapName)
			this.value = attribs.settings.mapName;
	}

	updateName()
	{
		this.value = this.settings.map?.data?.settings?.Name || this.settings.map.map;
	}
};
