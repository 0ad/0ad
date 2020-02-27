GameSettingControls.MapSelection = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;

		this.previousMapType = undefined;
		this.previousMapFilter = undefined;

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

	onGameAttributesChange()
	{
		if (!g_GameAttributes.mapType || !g_GameAttributes.mapFilter)
			return;

		this.updateMapList();

		if (!this.gameSettingsControl.autostart &&
			this.values &&
			this.values.file.indexOf(g_GameAttributes.map || undefined) == -1)
		{
			g_GameAttributes.map = this.values.file[0];
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (g_GameAttributes.map)
			this.setSelectedValue(g_GameAttributes.map);
	}

	updateMapList()
	{
		if (this.previousMapType &&
			this.previousMapType == g_GameAttributes.mapType &&
			this.previousMapFilter &&
			this.previousMapFilter == g_GameAttributes.mapFilter)
			return;

		Engine.ProfileStart("updateMapSelectionList");

		this.previousMapType = g_GameAttributes.mapType;
		this.previousMapFilter = g_GameAttributes.mapFilter;

		{
			let values =
				this.mapFilters.getFilteredMaps(
					g_GameAttributes.mapType,
					g_GameAttributes.mapFilter,
					false);

			values.sort(sortNameIgnoreCase);

			if (g_GameAttributes.mapType == "random")
				values.unshift(this.randomItem);

			this.values = prepareForDropdown(values);
		}

		this.dropdown.list = this.values.name;
		this.dropdown.list_data = this.values.file;
		Engine.ProfileStop();
	}

	onSelectionChange(itemIdx)
	{
		this.selectMap(this.values.file[itemIdx]);
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	/**
	 * @param mapPath - for example "maps/skirmishes/Acropolis Bay (2)"
	 */
	selectMap(mapPath)
	{
		if (g_GameAttributes.map == mapPath)
			return;

		Engine.ProfileStart("selectMap");

		// For scenario map, reset every setting per map selection
		// For skirmish and random maps, persist player choices
		if (g_GameAttributes.mapType == "scenario")
			g_GameAttributes = {
				"mapType": g_GameAttributes.mapType,
				"mapFilter": g_GameAttributes.mapFilter
			};

		g_GameAttributes.map = mapPath;
		this.gameSettingsControl.updateGameAttributes();

		Engine.ProfileStop();
	}

	getAutocompleteEntries()
	{
		return this.values.name;
	}

	onPickRandomItems()
	{
		if (g_GameAttributes.map == this.RandomMapId)
		{
			this.selectMap(pickRandom(this.values.file.slice(1)));
			this.gameSettingsControl.pickRandomItems();
		}
	}

	onGameAttributesFinalize()
	{
		// Copy map well known properties (and only well known properties)
		let mapData = this.mapCache.getMapData(g_GameAttributes.mapType, g_GameAttributes.map);

		if (g_GameAttributes.mapType == "random")
			g_GameAttributes.script = mapData.settings.Script;

		g_GameAttributes.settings.TriggerScripts = Array.from(new Set([
			...(g_GameAttributes.settings.TriggerScripts || []),
			...(mapData.settings.TriggerScripts || [])
		]));

		for (let property of this.MapSettings)
			if (mapData.settings[property] !== undefined)
				g_GameAttributes.settings[property] = mapData.settings[property];
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

GameSettingControls.MapSelection.prototype.MapSettings = [
	"CircularMap",
	"StartingTechnologies",
	"DisabledTechnologies",
	"DisabledTemplates",
	"StartingCamera",
	"Garrison",
	// Copy the map name so that the replay menu doesn't have to load hundreds of map descriptions on init
	// Also it allows determining the english mapname from the replay file if the map is not available.
	"Name"
];
