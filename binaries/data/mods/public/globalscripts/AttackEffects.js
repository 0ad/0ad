/**
 * This class provides a cache for accessing attack effects stored in JSON files.
 */
class AttackEffects
{
	constructor()
	{
		let effectsDataObj = {};
		this.effectReceivers = [];
		this.effectSounds = {};

		for (let filename of Engine.ListDirectoryFiles("simulation/data/attack_effects", "*.json", false))
		{
			let data = Engine.ReadJSONFile(filename);
			if (!data)
				continue;

			if (effectsDataObj[data.code])
			{
				error("Encountered two effect types with the code " + data.name + ".");
				continue;
			}

			effectsDataObj[data.code] = data;

			this.effectReceivers.push({
				"type": data.code,
				"IID": data.IID,
				"method": data.method
			});
			this.effectSounds[data.code] = data.sound || "";
		}

		let effDataSort = (a, b) => a.order < b.order ? -1 : a.order > b.order ? 1 : 0;
		let effSort = (a, b) => effDataSort(
			effectsDataObj[a.type],
			effectsDataObj[b.type]
		);
		this.effectReceivers.sort(effSort);

		deepfreeze(this.effectReceivers);
		deepfreeze(this.effectSounds);
	}

	/**
	 * @return {Object[]} - The effects possible with their data.
	 */
	Receivers()
	{
		return this.effectReceivers;
	}

	/**
	 * @param {string} type - The type of effect to get the receiving sound for.
	 * @return {string} - The name of the soundgroup to play.
	 */
	GetSound(type)
	{
		return this.effectSounds[type] || "";
	}
}
