GameSettingControls.StartingResources = class StartingResources extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.dropdown.list = g_StartingResources.Title;
		this.dropdown.list_data = g_StartingResources.Resources;

		this.sprintfArgs = {};

		g_GameSettings.startingResources.watch(() => this.render(), ["resources", "perPlayer"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	onHoverChange()
	{
		let tooltip = this.Tooltip;
		if (this.dropdown.hovered != -1)
		{
			this.sprintfArgs.resources = g_StartingResources.Resources[this.dropdown.hovered];
			tooltip = sprintf(this.HoverTooltip, this.sprintfArgs);
		}
		this.dropdown.tooltip = tooltip;
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario" && !g_GameSettings.startingResources.perPlayer);
		if (g_GameSettings.startingResources.perPlayer)
			this.label.caption = this.PerPlayerCaption;
		else
			this.setSelectedValue(g_GameSettings.startingResources.resources);
	}

	getAutocompleteEntries()
	{
		return g_StartingResources.Title;
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.startingResources.setResources(g_StartingResources.Resources[itemIdx]);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.StartingResources.prototype.TitleCaption =
	translate("Starting Resources");

GameSettingControls.StartingResources.prototype.Tooltip =
	translate("Select the game's starting resources.");

GameSettingControls.StartingResources.prototype.HoverTooltip =
	translate("Initial amount of each resource: %(resources)s.");

GameSettingControls.StartingResources.prototype.PerPlayerCaption =
	translateWithContext("starting resources", "Per Player");

GameSettingControls.StartingResources.prototype.AutocompleteOrder = 0;
