GameSettingControls.Landscape = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;
		this.lastLandscape = undefined;
		this.mapData = undefined;
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values.Description[this.dropdown.hovered] || this.Tooltip;
	}

	onMapChange(mapData)
	{
		this.mapData = mapData;

		if (mapData && mapData.settings && mapData.settings.Landscapes)
		{
			let randomItems = [];
			for (let item of this.RandomItems)
				if (item.Id == "random" || mapData.settings.Landscapes.land && mapData.settings.Landscapes.naval)
					randomItems.push({
						"Id": item.Id,
						"Name": setStringTags(item.Name, this.RandomItemTags),
						"Description": item.Description,
						"Preview": mapData.settings.Preview || this.mapCache.DefaultPreview
					});

			let sort = (item1, item2) => item1.Name > item2.Name;

			this.values = prepareForDropdown([
				...randomItems,
				...mapData.settings.Landscapes.land.map(this.translateItem).sort(sort),
				...mapData.settings.Landscapes.naval.map(this.translateItem).sort(sort)
			]);

			this.dropdown.list = this.values.Name;
			this.dropdown.list_data = this.values.Id;
		}
		else
			this.values = undefined;

		this.lastLandscape = undefined;

		this.setHidden(!this.values);
	}

	translateItem(item)
	{
		return {
			"Id": item.Id,
			"Name": translate(item.Name),
			"Description": translate(item.Description),
			"Preview": item.Preview
		};
	}

	onGameAttributesChange()
	{
		if (this.values)
		{
			if (this.values.Id.indexOf(g_GameAttributes.settings.Landscape || undefined) == -1)
			{
				g_GameAttributes.settings.Landscape = "random";
				this.gameSettingsControl.updateGameAttributes();
			}

			if (this.lastLandscape != g_GameAttributes.settings.Landscape)
			{
				g_GameAttributes.settings.Preview = this.values.Preview[this.values.Id.indexOf(g_GameAttributes.settings.Landscape)];
				this.lastLandscape = g_GameAttributes.settings.Biome;
			}
		}
		else if (g_GameAttributes.settings.Landscape !== undefined)
		{
			delete g_GameAttributes.settings.Landscape;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (g_GameAttributes.settings.Landscape)
			this.setSelectedValue(g_GameAttributes.settings.Landscape);
	}

	getAutocompleteEntries()
	{
		if (!this.values)
			return undefined;

		let entries = [];
		for (let i = 0; i < this.values.Id.length; ++i)
			if (!this.values.Id[i].startsWith("random"))
				entries.push(this.values.Name[i]);

		return entries;
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.Landscape = this.values.Id[itemIdx];

		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onPickRandomItems()
	{
		if (!this.mapData)
			return;

		let items;
		let landscapes = this.mapData.settings.Landscapes;

		switch (g_GameAttributes.settings.Landscape || undefined)
		{
		case "random":
			items = [...landscapes.land, ...landscapes.naval];
			break;
		case "random_land":
			items = landscapes.land;
			break;
		case "random_naval":
			items = landscapes.naval;
			break;
		default:
			return;
		}

		g_GameAttributes.settings.Landscape = pickRandom(items).Id;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.pickRandomItems();
	}
};

GameSettingControls.Landscape.prototype.TitleCaption =
	translate("Landscape");

GameSettingControls.Landscape.prototype.Tooltip =
	translate("Select one of the landscapes of this map.");

GameSettingControls.Landscape.prototype.RandomItems =
[
	{
		"Id": "random",
		"Name": translateWithContext("landscape selection", "Random Land or Naval"),
		"Description": translateWithContext("landscape selection", "Select a random land or naval map generation.")
	},
	{
		"Id": "random_land",
		"Name": translateWithContext("landscape selection", "Random Land"),
		"Description": translateWithContext("landscape selection", "Select a random land map generation.")
	},
	{
		"Id": "random_naval",
		"Name": translateWithContext("landscape selection", "Random Naval"),
		"Description": translateWithContext("landscape selection", "Select a random naval map generation.")
	}
];

GameSettingControls.Landscape.prototype.AutocompleteOrder = 0;
