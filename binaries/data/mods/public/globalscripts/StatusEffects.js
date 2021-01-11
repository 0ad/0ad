/**
 * This class provides a cache for accessing status effects metadata stored in JSON files.
 * This class must be initialised before using, as initialising it directly in globalscripts would
 * introduce disk I/O every time e.g. a GUI page is loaded.
 */
class StatusEffectsMetadata
{
	constructor()
	{
		this.statusEffectData = {};

		let files = Engine.ListDirectoryFiles("simulation/data/status_effects", "*.json", false);
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

			this.statusEffectData[data.code] = {
				"applierTooltip": data.applierTooltip || "",
				"code": data.code,
				"icon": data.icon || "default",
				"statusName": data.statusName || data.code,
				"receiverTooltip": data.receiverTooltip || ""
			};
		}

		deepfreeze(this.statusEffectData);
	}

	/**
	 * @param {string} code - The code of the Status Effect.
	 * @return {Object} - The JSON data corresponding to the code.
	 */
	getData(code)
	{
		if (this.statusEffectData[code])
			return this.statusEffectData[code];

		warn("No status effects data found for: " + code + ".");
		return {};
	}

	getApplierTooltip(code)
	{
		return this.getData(code).applierTooltip;
	}

	getIcon(code)
	{
		return this.getData(code).icon;
	}

	getName(code)
	{
		return this.getData(code).statusName;
	}

	getReceiverTooltip(code)
	{
		return this.getData(code).receiverTooltip;
	}
}
