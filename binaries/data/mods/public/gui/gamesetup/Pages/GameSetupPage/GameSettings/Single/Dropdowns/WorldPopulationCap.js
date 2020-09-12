GameSettingControls.WorldPopulationCap = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.dropdown.list = g_WorldPopulationCapacities.Title;
		this.dropdown.list_data = g_WorldPopulationCapacities.Population;

		this.sprintfArgs = {};
	}

	onMapChange(mapData)
	{
		let mapValue;
		if (mapData &&
			mapData.settings &&
			mapData.settings.WorldPopulationCap !== undefined)
			mapValue = mapData.settings.WorldPopulationCap;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.WorldPopulationCap)
		{
			g_GameAttributes.settings.WorldPopulationCap = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}

		this.setEnabled(g_GameAttributes.mapType != "scenario");
	}

	onGameAttributesChange()
	{
		if (g_GameAttributes.settings.WorldPopulation)
		{
			this.setHidden(false);
			if (g_GameAttributes.settings.WorldPopulationCap === undefined)
			{
				g_GameAttributes.settings.WorldPopulationCap = g_WorldPopulationCapacities.Population[g_WorldPopulationCapacities.Default];
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else
		{
			this.setHidden(true);
			g_GameAttributes.settings.WorldPopulationCap = undefined;
		}
	}

	onGameAttributesBatchChange()
	{
		this.setSelectedValue(g_GameAttributes.settings.WorldPopulationCap);
	}

	onHoverChange()
	{
		let tooltip = this.Tooltip;
		if (this.dropdown.hovered != -1)
		{
			let popCap = g_WorldPopulationCapacities.Population[this.dropdown.hovered];
			if (popCap >= this.WorldPopulationCapacityRecommendation)
			{
				this.sprintfArgs.popCap = popCap;
				tooltip = setStringTags(sprintf(this.HoverTooltip, this.sprintfArgs), this.HoverTags);
			}
		}
		this.dropdown.tooltip = tooltip;
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.WorldPopulationCap = g_WorldPopulationCapacities.Population[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.WorldPopulationCap.prototype.TitleCaption =
	translate("World Population Cap");

GameSettingControls.WorldPopulationCap.prototype.Tooltip =
	translate("Select world population limit.");

GameSettingControls.WorldPopulationCap.prototype.HoverTooltip =
	translate("Warning: There might be performance issues if %(popCap)s population is reached.");

GameSettingControls.WorldPopulationCap.prototype.HoverTags = {
	"color": "orange"
};

/**
 * Total number of units that the engine can run with smoothly.
 */
GameSettingControls.WorldPopulationCap.prototype.WorldPopulationCapacityRecommendation = 1200;
