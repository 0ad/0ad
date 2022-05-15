GameSettings.prototype.Attributes.TriggerScripts = class TriggerScripts extends GameSetting
{
	init()
	{
		this.customScripts = new Set();
		this.victoryScripts = new Set();
		this.mapScripts = new Set();
		this.settings.map.watch(() => this.updateMapScripts(), ["map"]);
		this.settings.victoryConditions.watch(() => this.updateVictoryScripts(), ["active"]);
	}

	toInitAttributes(attribs)
	{
		attribs.settings.TriggerScripts = Array.from(this.customScripts);
	}

	fromInitAttributes(attribs)
	{
		if (!!this.getLegacySetting(attribs, "TriggerScripts"))
			this.customScripts = new Set(this.getLegacySetting(attribs, "TriggerScripts"));
	}

	updateVictoryScripts()
	{
		let setting = this.settings.victoryConditions;
		let scripts = new Set();
		for (let cond of setting.active)
			setting.conditions[cond].Scripts.forEach(script => scripts.add(script));
		this.victoryScripts = scripts;
	}

	updateMapScripts()
	{
		if (!this.settings.map.data || !this.settings.map.data.settings ||
			!this.settings.map.data.settings.TriggerScripts)
		{
			this.mapScripts = new Set();
			return;
		}
		this.mapScripts = new Set(this.settings.map.data.settings.TriggerScripts);
	}

	onFinalizeAttributes(attribs)
	{
		const scripts = this.customScripts;
		for (const elem of this.victoryScripts)
			scripts.add(elem);
		for (const elem of this.mapScripts)
			scripts.add(elem);
		attribs.settings.TriggerScripts = Array.from(scripts);
	}
};
