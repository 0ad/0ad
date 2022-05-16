/**
 * The game settings are split in subclasses that are only responsible
 * for a logical subset of settings.
 * These are observables so updates are automated.
 */
class GameSetting extends Observable /* ProfilableMixin(Observable) /* Replace to profile automatically. */
{
	constructor(settings)
	{
		super();
		this.settings = settings;
	}

	getDefaultValue(settingsProp, dataProp)
	{
		for (let index in g_Settings[settingsProp])
			if (g_Settings[settingsProp][index].Default)
				return g_Settings[settingsProp][index][dataProp];
		return undefined;
	}

	getLegacySetting(attrib, name)
	{
		if (!attrib || !attrib.settings || attrib.settings[name] === undefined)
			return undefined;
		return attrib.settings[name];
	}

	getMapSetting(name)
	{
		if (!this.settings.map.data || !this.settings.map.data.settings ||
			this.settings.map.data.settings[name] === undefined)
			return undefined;
		return this.settings.map.data.settings[name];
	}
}
