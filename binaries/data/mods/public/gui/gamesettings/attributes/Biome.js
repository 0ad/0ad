GameSettings.prototype.Attributes.Biome = class Biome extends GameSetting
{
	init()
	{
		this.biomes = loadBiomes();
		this.biomeData = {};
		for (let biome of this.biomes)
			this.biomeData[biome.Id] = biome;
		this.cachedMapData = undefined;

		this.biome = undefined;
		// NB: random is always available.
		this.available = new Set();

		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (!this.biome)
			return;
		attribs.settings.Biome = this.biome;
	}

	fromInitAttributes(attribs)
	{
		if (!this.getLegacySetting(attribs, "Biome"))
			this.setBiome(undefined);
		else
			this.setBiome(this.getLegacySetting(attribs, "Biome"));
	}

	onMapChange()
	{
		let mapData = this.settings.map.data;
		if (mapData && mapData.settings && mapData.settings.SupportedBiomes !== undefined)
		{
			if (mapData.settings.SupportedBiomes === this.cachedMapData)
				return;
			this.cachedMapData = mapData.settings.SupportedBiomes;
			this.available = new Set(this.biomes.filter(biome => biome.Id.indexOf(mapData.settings.SupportedBiomes) !== -1)
				.map(biome => biome.Id));
			this.biome = "random";
		}
		else if (this.cachedMapData !== undefined)
		{
			this.cachedMapData = undefined;
			this.available = new Set();
			this.biome = undefined;
		}
	}

	setBiome(biome)
	{
		// TODO: more validation.
		if (this.available.size)
			this.biome = biome || "random";
		else
			this.biome = undefined;
	}

	getAvailableBiomeData()
	{
		return Array.from(this.available).map(biome => this.biomeData[biome]);
	}

	getData()
	{
		if (!this.biome)
			return undefined;
		return this.biomeData[this.biome];
	}

	pickRandomItems()
	{
		// If the map is random, we need to wait until it selects to know if we need to pick a biome.
		if (this.settings.map.map === "random")
			return true;

		if (this.biome !== "random")
			return false;
		this.biome = pickRandom(this.available);
		return true;
	}
};
