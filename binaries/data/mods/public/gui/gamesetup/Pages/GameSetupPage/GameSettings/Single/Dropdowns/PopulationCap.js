GameSettingControls.PopulationCap = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.perPlayer = false;

		this.dropdown.list = g_PopulationCapacities.Title;
		this.dropdown.list_data = g_PopulationCapacities.Population;

		this.sprintfArgs = {};
	}

	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.PopulationCap || undefined;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.PopulationCap)
		{
			g_GameAttributes.settings.PopulationCap = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}

		let isScenario = g_GameAttributes.mapType == "scenario";

		this.perPlayer =
			isScenario &&
			mapData.settings.PlayerData &&
			mapData.settings.PlayerData.some(pData => pData && pData.PopulationLimit !== undefined);

		this.setEnabled(!isScenario && !this.perPlayer);

		if (this.perPlayer)
			this.label.caption = this.PerPlayerCaption;
	}

	onGameAttributesChange()
	{
		if (g_GameAttributes.settings.PopulationCap === undefined)
		{
			g_GameAttributes.settings.PopulationCap = g_PopulationCapacities.Population[g_PopulationCapacities.Default];
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (!this.perPlayer)
			this.setSelectedValue(g_GameAttributes.settings.PopulationCap);
	}

	onHoverChange()
	{
		let tooltip = this.Tooltip;
		if (this.dropdown.hovered != -1)
		{
			let popCap = g_PopulationCapacities.Population[this.dropdown.hovered];
			let players = g_GameAttributes.settings.PlayerData.length;
			if (popCap * players >= this.PopulationCapacityRecommendation)
			{
				this.sprintfArgs.players = players;
				this.sprintfArgs.popCap = popCap;
				tooltip = setStringTags(sprintf(this.HoverTooltip, this.sprintfArgs), this.HoverTags);
			}
		}
		this.dropdown.tooltip = tooltip;
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.PopulationCap = g_PopulationCapacities.Population[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.PopulationCap.prototype.TitleCaption =
	translate("Population Cap");

GameSettingControls.PopulationCap.prototype.Tooltip =
	translate("Select population limit.");

GameSettingControls.PopulationCap.prototype.PerPlayerCaption =
	translateWithContext("population limit", "Per Player");

GameSettingControls.PopulationCap.prototype.HoverTooltip =
	translate("Warning: There might be performance issues if all %(players)s players reach %(popCap)s population.");

GameSettingControls.PopulationCap.prototype.HoverTags = {
	"color": "orange"
};

/**
 * Total number of units that the engine can run with smoothly.
 * It means a 4v4 with 150 population can still run nicely, but more than that might "lag".
 */
GameSettingControls.PopulationCap.prototype.PopulationCapacityRecommendation = 1200;
