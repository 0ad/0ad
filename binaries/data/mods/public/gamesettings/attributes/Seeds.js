GameSettings.prototype.Attributes.Seeds = class Seeds extends GameSetting
{
	init()
	{
		this.seed = "random";
		this.AIseed = "random";
	}

	toInitAttributes(attribs)
	{
		attribs.settings.Seed = this.seed == "random" ? this.seed : +this.seed;
		attribs.settings.AISeed = this.AIseed == "random" ? this.AIseed : +this.AIseed;
	}

	fromInitAttributes(attribs)
	{
		if (this.getLegacySetting(attribs, "Seed") !== undefined)
			this.seed = this.getLegacySetting(attribs, "Seed");
		if (this.getLegacySetting(attribs, "AISeed") !== undefined)
			this.AIseed = this.getLegacySetting(attribs, "AISeed");
	}

	pickRandomItems()
	{
		let picked = false;
		if (this.seed === "random")
		{
			this.seed = randIntExclusive(0, Math.pow(2, 32));
			picked = true;
		}

		if (this.AIseed === "random")
		{
			this.AIseed = randIntExclusive(0, Math.pow(2, 32));
			picked = true;
		}
		return picked;
	}
};
