/**
 * Combines the worldPopulation and regular population cap.
 * At the moment those are incompatible so this makes sense.
 * TODO: Should there be a dialog allowing per-player pop limits?
 */
GameSettings.prototype.Attributes.Population = class Population extends GameSetting
{
	init()
	{
		this.popDefault = this.getDefaultValue("PopulationCapacities", "Population") || 200;
		this.worldPopDefault = this.getDefaultValue("WorldPopulationCapacities", "Population") || 800;

		this.perPlayer = false;
		this.useWorldPop = false;
		this.cap = this.popDefault;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.perPlayer)
		{
			if (!attribs.settings.PlayerData)
				attribs.settings.PlayerData = [];
			while (attribs.settings.PlayerData.length < this.perPlayer.length)
				attribs.settings.PlayerData.push({});
			for (let i in this.perPlayer)
				if (this.perPlayer[i])
					attribs.settings.PlayerData[i].PopulationLimit = this.perPlayer[i];
		}
		if (this.useWorldPop)
		{
			attribs.settings.WorldPopulation = true;
			attribs.settings.WorldPopulationCap = this.cap;
		}
		else
			attribs.settings.PopulationCap = this.cap;
	}

	fromInitAttributes(attribs)
	{
		if (!!this.getLegacySetting(attribs, "WorldPopulation"))
			this.setPopCap(true, this.getLegacySetting(attribs, "WorldPopulationCap"));
		else if (!!this.getLegacySetting(attribs, "PopulationCap"))
			this.setPopCap(false, this.getLegacySetting(attribs, "PopulationCap"));
	}

	onMapChange()
	{
		this.perPlayer = undefined;
		if (this.settings.map.type != "scenario")
			return;
		if (this.getMapSetting("PlayerData")?.some(data => data.PopulationLimit))
			this.perPlayer = this.getMapSetting("PlayerData").map(data => data.PopulationLimit || undefined);
		else if (this.getMapSetting("WorldPopulation"))
			this.setPopCap(true, +this.getMapSetting("WorldPopulationCap"));
		else
			this.setPopCap(false, +this.getMapSetting("PopulationCap"));
	}

	setPopCap(worldPop, cap = undefined)
	{
		if (worldPop != this.useWorldPop)
			this.cap = undefined;

		this.useWorldPop = worldPop;

		if (!!cap)
			this.cap = cap;
		else if (!this.cap && !this.useWorldPop)
			this.cap = this.popDefault;
		else if (!this.cap && this.useWorldPop)
			this.cap = this.worldPopDefault;
	}
};
