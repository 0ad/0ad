/**
 * This class provides a cache for accessing status effects metadata stored in JSON files.
 * Note that status effects need not be defined in JSON files to be handled in-game.
 * This class must be initialised before using, as initialising it directly in globalscripts would
 * introduce disk I/O every time e.g. a GUI page is loaded.
 */
class StatusEffectsMetadata
{
	constructor()
	{
		this.statusEffectData = {};

		let files = Engine.ListDirectoryFiles("simulation/data/template_helpers/status_effects", "*.json", false);
		for (let filename of files)
		{
			let data = Engine.ReadJSONFile(filename);
			if (!data)
				continue;

			if (data.code in this.statusEffectData)
			{
				error("Encountered two status effects with the code " + data.code);
				continue;
			}

			this.statusEffectData[data.code] = data;
		}
	}

	/**
	 * @returns the default data for @param code status effects, augmented with the given template data,
	 * or simply @param templateData if the code is not found in JSON files.
	 */
	augment(code, templateData)
	{
		if (!templateData && this.statusEffectData[code])
			return this.statusEffectData[code];

		if (this.statusEffectData[code])
			return Object.assign({}, this.statusEffectData[code], templateData);

		return templateData;
	}
}
