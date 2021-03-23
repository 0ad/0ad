/**
 * Maptype design:
 * Scenario maps have fixed terrain and all settings predetermined.
 * Skirmish maps have fixed terrain, playercount but settings are free.
 * For random maps, settings are fully determined by the player and the terrain is generated based on them.
 */
GameSettingControls.MapType = class MapType extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.dropdown.list = g_MapTypes.Title;
		this.dropdown.list_data = g_MapTypes.Name;

		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	onLoad()
	{
		// Select a default map type if none are currently chosen.
		// This in cascade will select a default filter and a default map.
		if (!g_GameSettings.map.type)
			g_GameSettings.map.setType(g_MapTypes.Name[g_MapTypes.Default]);
	}

	onHoverChange()
	{
		this.dropdown.tooltip = g_MapTypes.Description[this.dropdown.hovered] || this.Tooltip;
	}

	render()
	{
		this.setSelectedValue(g_GameSettings.map.type);
	}

	getAutocompleteEntries()
	{
		return g_MapTypes.Title;
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.map.setType(g_MapTypes.Name[itemIdx]);
		this.gameSettingsController.setNetworkInitAttributes();
	}
};

GameSettingControls.MapType.prototype.TitleCaption =
	translate("Map Type");

GameSettingControls.MapType.prototype.Tooltip =
	translate("Select a map type.");

GameSettingControls.MapType.prototype.AutocompleteOrder = 0;
