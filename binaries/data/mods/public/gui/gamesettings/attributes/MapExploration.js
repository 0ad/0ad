GameSettings.prototype.Attributes.MapExploration = class MapExploration extends GameSetting
{
	init()
	{
		this.resetAll();
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	resetAll()
	{
		this.explored = false;
		this.revealed = false;
		this.allied = false;
	}

	toInitAttributes(attribs)
	{
		attribs.settings.RevealMap = this.revealed;
		attribs.settings.ExploreMap = this.explored;
		attribs.settings.AllyView = this.allied;
	}

	fromInitAttributes(attribs)
	{
		this.explored = !!this.getLegacySetting(attribs, "ExploreMap");
		this.revealed = !!this.getLegacySetting(attribs, "RevealMap");
		this.allied = !!this.getLegacySetting(attribs, "AllyView");
	}

	onMapChange(mapData)
	{
		if (this.settings.map.type != "scenario")
			return;
		this.resetAll();
		this.setExplored(this.getMapSetting("ExploreMap"));
		this.setRevealed(this.getMapSetting("RevealMap"));
		this.setAllied(this.getMapSetting("AllyView"));
	}

	setExplored(enabled)
	{
		if (enabled === undefined)
			return;
		this.explored = enabled;
		this.revealed = this.revealed && this.explored;
	}

	setRevealed(enabled)
	{
		if (enabled === undefined)
			return;
		this.revealed = enabled;
		this.explored = this.explored || this.revealed;
	}

	setAllied(enabled)
	{
		if (enabled === undefined)
			return;
		this.allied = enabled;
	}
};
