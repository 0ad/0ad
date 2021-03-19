GameSettingControls.PopulationCap = class PopulationCap extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.perPlayer = false;

		this.dropdown.list = g_PopulationCapacities.Title;
		this.dropdown.list_data = g_PopulationCapacities.Population;

		this.sprintfArgs = {};

		g_GameSettings.population.watch(() => this.render(), ["useWorldPop", "cap", "perPlayer"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setHidden(g_GameSettings.population.useWorldPop);
		this.setEnabled(!g_GameSettings.map.type == "scenario" && !g_GameSettings.population.perPlayer);
		if (g_GameSettings.population.perPlayer)
			this.label.caption = this.PerPlayerCaption;
		else
			this.setSelectedValue(g_GameSettings.population.cap);
	}

	onHoverChange()
	{
		let tooltip = this.Tooltip;
		if (this.dropdown.hovered != -1)
		{
			let popCap = g_PopulationCapacities.Population[this.dropdown.hovered];
			let players = g_GameSettings.playerCount.nbPlayers;
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
		g_GameSettings.population.setPopCap(false, g_PopulationCapacities.Population[itemIdx]);
		this.gameSettingsControl.setNetworkInitAttributes();
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
