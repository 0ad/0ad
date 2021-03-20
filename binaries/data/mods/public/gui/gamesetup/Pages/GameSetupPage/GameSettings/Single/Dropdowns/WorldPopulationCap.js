GameSettingControls.WorldPopulationCap = class WorldPopulationCap extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.dropdown.list = g_WorldPopulationCapacities.Title;
		this.dropdown.list_data = g_WorldPopulationCapacities.Population;

		this.sprintfArgs = {};

		g_GameSettings.population.watch(() => this.render(), ["useWorldPop", "cap"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario");
		this.setHidden(!g_GameSettings.population.useWorldPop);
		this.setSelectedValue(g_GameSettings.population.cap);
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
		g_GameSettings.population.setPopCap(true, g_WorldPopulationCapacities.Population[itemIdx]);
		this.gameSettingsControl.setNetworkInitAttributes();
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
