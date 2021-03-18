GameSettings.prototype.Attributes.TriggerScripts = class TriggerScripts extends GameSetting
{
	init()
	{
		this.victory = new Set();
		this.map = new Set();
		this.settings.map.watch(() => this.updateMapScripts(), ["map"]);
		this.settings.victoryConditions.watch(() => this.updateVictoryScripts(), ["active"]);
	}

	toInitAttributes(attribs)
	{
		let scripts = new Set(this.victory);
		for (let elem of this.map)
			scripts.add(elem);
		attribs.settings.TriggerScripts = Array.from(scripts);
	}

	/**
	 * Exceptionally, this setting has no Deserialize: it's entirely determined from other settings.
	 */

	updateVictoryScripts()
	{
		let setting = this.settings.victoryConditions;
		let scripts = new Set();
		for (let cond of setting.active)
			setting.conditions[cond].Scripts.forEach(script => scripts.add(script));
		this.victory = scripts;
	}

	updateMapScripts()
	{
		if (!this.settings.map.data || !this.settings.map.data.settings ||
			!this.settings.map.data.settings.TriggerScripts)
		{
			this.map = new Set();
			return;
		}
		this.map = new Set(this.settings.map.data.settings.TriggerScripts);
	}
};
