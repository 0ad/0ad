/**
 * Map name.
 * This is usually just the regular map name, but can be overwritten.
 */
GameSettings.prototype.Attributes.MapName = class MapName extends GameSetting
{
	init()
	{
	}

	toInitAttributes(attribs)
	{
		if (this.value)
			attribs.settings.Name = this.value;
		else
		{
			// Copy from the map data by default - this helps make InitAttributes self-sufficient,
			// which is nice for replays / saved games.
			// Fallback to the map name to avoid 'undefined' errors.
			attribs.settings.Name = this.settings.map?.data?.settings?.Name || this.settings.map.map;
		}
	}

	fromInitAttributes(attribs)
	{
		// Ser/Deser from a different attribute name as a poor man's not-persisted-setting.
		// TODO: split this off more properly.
		if (attribs.mapName)
			this.value = attribs.mapName;
	}

	set(name)
	{
		this.value = name;
	}
};
