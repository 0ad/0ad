GameSettings.prototype.Attributes.TeamPlacement = class TeamPlacement extends GameSetting
{
	init()
	{
		this.available = undefined;
		this.value = undefined;
		this.settings.map.watch(() => this.onMapChange(), ["map"]);
	}

	toInitAttributes(attribs)
	{
		if (this.value !== undefined)
			attribs.settings.TeamPlacement = this.value;
	}

	fromInitAttributes(attribs)
	{
		if (!!this.getLegacySetting(attribs, "TeamPlacement"))
			this.value = this.getLegacySetting(attribs, "TeamPlacement");
	}

	onMapChange()
	{
		if (!this.getMapSetting("TeamPlacements"))
		{
			this.value = undefined;
			this.available = undefined;
			return;
		}
		// TODO: should probably validate that they fit one of the known schemes.
		this.available = this.getMapSetting("TeamPlacements");
		this.value = "random";
	}

	setValue(val)
	{
		this.value = val;
	}

	pickRandomItems()
	{
		// If the map is random, we need to wait until it is selected.
		if (this.settings.map.map === "random")
			return true;

		if (this.value !== "random")
			return false;
		this.value = pickRandom(this.available).Id;
		return true;
	}
};


GameSettings.prototype.Attributes.TeamPlacement.prototype.StartingPositions = [
	{
		"Id": "radial",
		"Name": translateWithContext("team placement", "Circle"),
		"Description": translate("Allied players are grouped and placed with opposing players on one circle spanning the map.")
	},
	{
		"Id": "line",
		"Name": translateWithContext("team placement", "Line"),
		"Description": translate("Allied players are placed in a linear pattern."),
	},
	{
		"Id": "randomGroup",
		"Name": translateWithContext("team placement", "Random Group"),
		"Description": translate("Allied players are grouped, but otherwise placed randomly on the map."),
	},
	{
		"Id": "stronghold",
		"Name": translateWithContext("team placement", "Stronghold"),
		"Description": translate("Allied players are grouped in one random place of the map."),
	}
];
