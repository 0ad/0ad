GameSettings.prototype.Attributes.Seeds = class Seeds extends GameSetting
{
	init()
	{
		this.seed = 0;
		this.AIseed = 0;
	}

	toInitAttributes(attribs)
	{
		// Seed is used for map generation and simulation.
		attribs.settings.Seed = this.seed;
		attribs.settings.AISeed = this.AIseed;
	}

	fromInitAttributes(attribs)
	{
		// Seed is used for map generation and simulation.
		if (this.getLegacySetting(attribs, "Seed") !== undefined)
			this.seed = this.getLegacySetting(attribs, "Seed");
		if (this.getLegacySetting(attribs, "AISeed") !== undefined)
			this.AIseed = this.getLegacySetting(attribs, "AISeed");
	}

	pickRandomItems()
	{
		this.seed = randIntExclusive(0, Math.pow(2, 32));
		this.AIseed = randIntExclusive(0, Math.pow(2, 32));
	}
};
