/**
 * Maptype design:
 * Scenario maps have fixed terrain and all settings predetermined.
 * Skirmish maps have fixed terrain, playercount but settings are free.
 * For random maps, settings are fully determined by the player and the terrain is generated based on them.
 */
GameSettingControls.MapType = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.dropdown.list = g_MapTypes.Title;
		this.dropdown.list_data = g_MapTypes.Name;
	}

	onHoverChange()
	{
		this.dropdown.tooltip = g_MapTypes.Description[this.dropdown.hovered] || this.Tooltip;
	}

	onGameAttributesChange()
	{
		if (g_MapTypes.Name.indexOf(g_GameAttributes.mapType || undefined) == -1)
		{
			g_GameAttributes.mapType = g_MapTypes.Name[g_MapTypes.Default];
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setSelectedValue(g_GameAttributes.mapType);
	}

	getAutocompleteEntries()
	{
		return g_MapTypes.Title;
	}

	onSelectionChange(itemIdx)
	{
		let mapType = g_MapTypes.Name[itemIdx];

		if (g_GameAttributes.mapType == mapType)
			return;

		if (mapType == "scenario")
			g_GameAttributes = {
				"mapFilter": g_GameAttributes.mapFilter
			};

		g_GameAttributes.mapType = mapType;

		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.MapType.prototype.TitleCaption =
	translate("Map Type");

GameSettingControls.MapType.prototype.Tooltip =
	translate("Select a map type.");

GameSettingControls.MapType.prototype.AutocompleteOrder = 0;
