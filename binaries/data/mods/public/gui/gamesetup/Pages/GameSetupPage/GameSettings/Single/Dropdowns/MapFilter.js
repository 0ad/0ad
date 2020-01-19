GameSettingControls.MapFilter = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;
		this.previousMapType = undefined;
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values.Description[this.dropdown.hovered] || this.Tooltip;
	}

	onGameAttributesChange()
	{
		this.checkMapTypeChange();

		if (this.values)
		{
			if (this.values.Name.indexOf(g_GameAttributes.mapFilter || undefined) == -1)
			{
				g_GameAttributes.mapFilter = this.values.Name[this.values.Default];
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.mapFilter !== undefined)
		{
			delete g_GameAttributes.mapFilter;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	checkMapTypeChange()
	{
		if (!g_GameAttributes.mapType || this.previousMapType == g_GameAttributes.mapType)
			return;

		Engine.ProfileStart("updateMapFilterList");
		this.previousMapType = g_GameAttributes.mapType;

		let values = prepareForDropdown(
			this.mapFilters.getAvailableMapFilters(
				g_GameAttributes.mapType));

		if (values.Name.length)
		{
			this.dropdown.list = values.Title;
			this.dropdown.list_data = values.Name;
			this.values = values;
		}
		else
			this.values = undefined;

		this.setHidden(!this.values);
		Engine.ProfileStop();
	}

	onGameAttributesBatchChange()
	{
		if (this.values && g_GameAttributes.mapFilter)
			this.setSelectedValue(g_GameAttributes.mapFilter);
	}

	getAutocompleteEntries()
	{
		return this.values && this.values.Title;
	}

	onSelectionChange(itemIdx)
	{
		if (this.values)
		{
			g_GameAttributes.mapFilter = this.values.Name[itemIdx];
			this.gameSettingsControl.updateGameAttributes();
			this.gameSettingsControl.setNetworkGameAttributes();
		}
	}

	onGameAttributesFinalize()
	{
		// The setting is only relevant to the gamesetup stage!
		delete g_GameAttributes.mapFilter;
	}
};

GameSettingControls.MapFilter.prototype.TitleCaption =
	translate("Map Filter");

GameSettingControls.MapFilter.prototype.Tooltip =
	translate("Select a map filter.");

GameSettingControls.MapFilter.prototype.AutocompleteOrder = 0;
