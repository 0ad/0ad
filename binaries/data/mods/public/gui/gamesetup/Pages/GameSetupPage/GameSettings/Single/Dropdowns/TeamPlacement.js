GameSettingControls.TeamPlacement = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);
		this.values = undefined;
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values.Description[this.dropdown.hovered] || this.Tooltip;
	}

	onMapChange(mapData)
	{
		if (mapData && mapData.settings && mapData.settings.TeamPlacements)
		{
			let randomItem = clone(this.RandomItem);
			randomItem.Name = setStringTags(randomItem.Name, this.RandomItemTags);

			let patterns = [randomItem];

			for (let pattern of mapData.settings.TeamPlacements)
				patterns.push(
					typeof pattern == "string" ?
						this.DefaultStartingPositions.find(pObj => pObj.Id == pattern) :
						{
							"Id": pattern.Id,
							"Name": translate(pattern.Name),
							"Description": translate(pattern.Description)
						});

			this.values = prepareForDropdown(patterns);

			this.dropdown.list = this.values.Name;
			this.dropdown.list_data = this.values.Id;
		}
		else
			this.values = undefined;
	}

	onGameAttributesChange()
	{
		if (this.values)
		{
			if (this.values.Id.indexOf(g_GameAttributes.settings.TeamPlacement || undefined) == -1)
			{
				g_GameAttributes.settings.TeamPlacement = this.values.Id[0];
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.TeamPlacement !== undefined)
		{
			delete g_GameAttributes.settings.TeamPlacement;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setHidden(!this.values);

		if (this.values)
			this.setSelectedValue(g_GameAttributes.settings.TeamPlacement);
	}

	getAutocompleteEntries()
	{
		return this.values && this.values.Name.slice(1);
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.TeamPlacement = this.values.Id[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onPickRandomItems()
	{
		if (!this.values || g_GameAttributes.settings.TeamPlacement != "random")
			return;

		g_GameAttributes.settings.TeamPlacement = pickRandom(this.values.Id.filter(id => id != "random"));
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.pickRandomItems();
	}
};

GameSettingControls.TeamPlacement.prototype.TitleCaption =
	translate("Team Placement");

GameSettingControls.TeamPlacement.prototype.Tooltip =
	translate("Select one of the starting position patterns of this map.");

GameSettingControls.TeamPlacement.prototype.RandomItem = {
	"Id": "random",
	"Name": translateWithContext("team placement", "Random"),
	"Description": translateWithContext("team placement", "Select a random team placement pattern when starting the game.")
};

GameSettingControls.TeamPlacement.prototype.DefaultStartingPositions = [
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

GameSettingControls.TeamPlacement.prototype.AutocompleteOrder = 0;
