GameSettingControls.MapFilter = class MapFilter extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;

		g_GameSettings.map.watch(() => this.checkMapTypeChange(), ["type"]);
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values.Description[this.dropdown.hovered] || this.Tooltip;
	}

	checkMapTypeChange()
	{
		if (!g_GameSettings.map.type)
			return;

		let values = prepareForDropdown(
			this.mapFilters.getAvailableMapFilters(
				g_GameSettings.map.type));

		if (values.Name.length)
		{
			this.dropdown.list = values.Title;
			this.dropdown.list_data = values.Name;
			this.values = values;
		}
		else
			this.values = undefined;

		if (this.values && this.values.Name.indexOf(this.gameSettingsControl.guiData.mapFilter.filter) === -1)
		{
			this.gameSettingsControl.guiData.mapFilter.filter = this.values.Name[this.values.Default];
			this.gameSettingsControl.setNetworkGameAttributes();
		}
		// Index may have changed, reset.
		this.setSelectedValue(this.gameSettingsControl.guiData.mapFilter.filter);

		this.setHidden(!this.values);
	}

	getAutocompleteEntries()
	{
		return this.values && this.values.Title;
	}

	onSelectionChange(itemIdx)
	{
		this.gameSettingsControl.guiData.mapFilter.filter = this.values.Name[itemIdx];
	}
};

GameSettingControls.MapFilter.prototype.TitleCaption =
	translate("Map Filter");

GameSettingControls.MapFilter.prototype.Tooltip =
	translate("Select a map filter.");

GameSettingControls.MapFilter.prototype.AutocompleteOrder = 0;
