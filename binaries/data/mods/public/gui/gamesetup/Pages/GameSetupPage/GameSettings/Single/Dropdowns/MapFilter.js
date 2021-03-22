GameSettingControls.MapFilter = class MapFilter extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;

		this.gameSettingsController.guiData.mapFilter.watch(() => this.render(), ["filter"]);
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

		if (this.values && this.values.Name.indexOf(this.gameSettingsController.guiData.mapFilter.filter) === -1)
		{
			this.gameSettingsController.guiData.mapFilter.filter = this.values.Name[this.values.Default];
			this.gameSettingsController.setNetworkInitAttributes();
		}
		this.render();
	}

	render()
	{
		// Index may have changed, reset.
		this.setSelectedValue(this.gameSettingsController.guiData.mapFilter.filter);
		this.setHidden(!this.values);
	}

	getAutocompleteEntries()
	{
		return this.values && this.values.Title;
	}

	onSelectionChange(itemIdx)
	{
		this.gameSettingsController.guiData.mapFilter.filter = this.values.Name[itemIdx];
	}
};

GameSettingControls.MapFilter.prototype.TitleCaption =
	translate("Map Filter");

GameSettingControls.MapFilter.prototype.Tooltip =
	translate("Select a map filter.");

GameSettingControls.MapFilter.prototype.AutocompleteOrder = 0;
