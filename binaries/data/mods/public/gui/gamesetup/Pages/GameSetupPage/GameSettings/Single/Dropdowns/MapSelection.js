GameSettingControls.MapSelection = class MapSelection extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;

		g_GameSettings.map.watch(() => this.render(), ["map"]);
		g_GameSettings.map.watch(() => this.updateMapList(), ["type"]);

		this.gameSettingsControl.guiData.mapFilter.watch(() => this.updateMapList(), ["filter"]);

		this.randomItem = {
			"file": this.RandomMapId,
			"name": setStringTags(this.RandomMapCaption, this.RandomItemTags),
			"description": this.RandomMapDescription
		};
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values.description[this.dropdown.hovered] || this.Tooltip;
	}

	render()
	{
		// Can happen with bad matchsettings
		if (this.values.file.indexOf(g_GameSettings.map.map) === -1)
		{
			g_GameSettings.map.selectMap(this.values.file[this.values.Default]);
			return;
		}
		this.setSelectedValue(g_GameSettings.map.map);
	}

	updateMapList()
	{
		Engine.ProfileStart("updateMapSelectionList");

		if (!g_GameSettings.map.type)
			return;

		{
			let values =
				this.mapFilters.getFilteredMaps(
					g_GameSettings.map.type,
					this.gameSettingsControl.guiData.mapFilter.filter,
					false);

			values.sort(sortNameIgnoreCase);

			if (g_GameSettings.map.type == "random")
				values.unshift(this.randomItem);

			this.values = prepareForDropdown(values);
		}

		this.dropdown.list = this.values.name;
		this.dropdown.list_data = this.values.file;

		g_GameSettings.map.setRandomOptions(this.values.file);

		// Reset the selected map.
		if (this.values.file.indexOf(g_GameSettings.map.map) === -1)
		{
			g_GameSettings.map.selectMap(this.values.file[this.values.Default]);
			this.gameSettingsControl.setNetworkInitAttributes();
		}
		// The index may have changed: reset.
		this.setSelectedValue(g_GameSettings.map.map);

		Engine.ProfileStop();
	}

	onSelectionChange(itemIdx)
	{
		// The triggering that happens on map change can be just slow enough
		// that the next event happens before we're done when scrolling,
		// and then the scrolling is not smooth since it can take arbitrarily long to render.
		// To avoid that, run the change on the next GUI tick, and only do one increment.
		// TODO: the problem is mostly that updating visibility can relayout the gamesetting,
		// which takes a few ms, but this could only be done once per frame anyways.
		// NB: this technically makes it possible to start the game without the change going through
		// but it's essentially impossible to trigger accidentally.
		if (this.reRenderTimeout)
			return;
		this.reRenderTimeout = setTimeout(() => {
			g_GameSettings.map.selectMap(this.values.file[itemIdx]);
			this.gameSettingsControl.setNetworkInitAttributes();
			delete this.reRenderTimeout;
		}, 0);
	}

	getAutocompleteEntries()
	{
		return this.values.name;
	}
};

GameSettingControls.MapSelection.prototype.TitleCaption =
	translate("Select Map");

GameSettingControls.MapSelection.prototype.Tooltip =
	translate("Select a map to play on.");

GameSettingControls.MapSelection.prototype.RandomMapId =
	"random";

GameSettingControls.MapSelection.prototype.RandomMapCaption =
	translateWithContext("map selection", "Random");

GameSettingControls.MapSelection.prototype.RandomMapDescription =
	translate("Pick any of the given maps at random.");

GameSettingControls.MapSelection.prototype.AutocompleteOrder = 0;
