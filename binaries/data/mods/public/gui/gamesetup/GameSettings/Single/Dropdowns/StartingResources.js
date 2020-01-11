GameSettingControls.StartingResources = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.dropdown.list = g_StartingResources.Title;
		this.dropdown.list_data = g_StartingResources.Resources;

		this.perPlayer = false;
		this.sprintfArgs = {};
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

	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.StartingResources || undefined;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.StartingResources)
		{
			g_GameAttributes.settings.StartingResources = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}

		let isScenario = g_GameAttributes.mapType == "scenario";

		this.perPlayer =
			isScenario &&
			mapData.settings.PlayerData &&
			mapData.settings.PlayerData.some(pData => pData && pData.Resources !== undefined);

		this.setEnabled(!isScenario && !this.perPlayer);

		if (this.perPlayer)
			this.label.caption = this.PerPlayerCaption;
	}

	onGameAttributesChange()
	{
		if (g_GameAttributes.settings.StartingResources === undefined)
		{
			g_GameAttributes.settings.StartingResources =
				g_StartingResources.Resources[g_StartingResources.Default];
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (!this.perPlayer)
			this.setSelectedValue(g_GameAttributes.settings.StartingResources);
	}

	getAutocompleteEntries()
	{
		return g_StartingResources.Title;
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.StartingResources = g_StartingResources.Resources[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
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
