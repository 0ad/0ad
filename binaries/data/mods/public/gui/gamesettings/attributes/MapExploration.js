GameSettings.prototype.Attributes.MapExploration = class MapExploration extends GameSetting
{
	init()
	{
		this.explored = false;
		this.revealed = false;
		this.allied = false;

		this.settings.map.watch(() => this.onMapChange(), ["map"]);
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
		this.setExplored(this.getMapSetting("ExploreMap"));
		this.setRevealed(this.getMapSetting("RevealMap"));
		this.setAllied(this.getMapSetting("AllyView"));
	}

	setExplored(enabled)
	{
		this.explored = enabled;
		this.revealed = this.revealed && this.explored;
	}

	setRevealed(enabled)
	{
		this.explored = this.explored || enabled;
		this.revealed = enabled;
		this.allied = this.allied || enabled;
	}

	setAllied(enabled)
	{
		this.allied = enabled;
		this.revealed = this.revealed && this.allied;
	}
};
