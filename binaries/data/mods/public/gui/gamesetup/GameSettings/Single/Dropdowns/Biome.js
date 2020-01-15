GameSettingControls.Biome = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;
		this.biomeValues = undefined;

		this.lastBiome = undefined;

		this.randomItem = {
			"Id": this.RandomBiomeId,
			"Title": setStringTags(this.RandomBiome, this.RandomItemTags),
			"Autocomplete": this.RandomBiome,
			"Description": this.RandomDescription
		};
	}

	onHoverChange()
	{
		this.dropdown.tooltip =
			this.values && this.values.Description && this.values.Description[this.dropdown.hovered] ||
			this.Tooltip;
	}

	onMapChange(mapData)
	{
		let available = g_GameAttributes.mapType == "random" &&
			mapData && mapData.settings && mapData.settings.SupportedBiomes || undefined;

		this.setHidden(!available);

		if (available)
		{
			Engine.ProfileStart("updateBiomeList");
			this.biomeValues =
				g_Settings.Biomes.filter(this.filterBiome(available)).map(this.createBiomeItem);

			this.values = prepareForDropdown([
				this.randomItem,
				...this.biomeValues
			]);

			this.dropdown.list = this.values.Title;
			this.dropdown.list_data = this.values.Id;
			Engine.ProfileStop();
		}
		else
			this.values = undefined;

		this.lastBiome = undefined;
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.mapType || !g_GameAttributes.map)
			return;

		if (this.values)
		{
			if (this.values.Id.indexOf(g_GameAttributes.settings.Biome || undefined) == -1)
			{
				g_GameAttributes.settings.Biome = this.RandomBiomeId;
				this.gameSettingsControl.updateGameAttributes();
			}

			if (this.lastBiome != g_GameAttributes.settings.Biome)
			{
				let biomePreviewFile =
					basename(g_GameAttributes.map) + "_" +
					basename(g_GameAttributes.settings.Biome || "") + ".png";

				if (Engine.TextureExists(this.mapCache.TexturesPath + this.mapCache.PreviewsPath + biomePreviewFile))
					g_GameAttributes.settings.Preview = biomePreviewFile;

				this.lastBiome = g_GameAttributes.settings.Biome;
			}
		}
		else if (g_GameAttributes.settings.Biome)
		{
			delete g_GameAttributes.settings.Biome;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (this.values)
			this.setSelectedValue(g_GameAttributes.settings.Biome);
	}

	filterBiome(available)
	{
		if (typeof available == "string")
			return biome => biome.Id.startsWith(available);

		return biome => available.indexOf(biome.Id) != -1;
	}

	createBiomeItem(biome)
	{
		return {
			"Id": biome.Id,
			"Title": biome.Title,
			"Autocomplete": biome.Title,
			"Description": biome.Description
		};
	}

	getAutocompleteEntries()
	{
		return this.values && this.values.Autocomplete;
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.Biome = this.values.Id[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onPickRandomItems()
	{
		if (this.values && g_GameAttributes.settings.Biome == this.RandomBiomeId)
		{
			g_GameAttributes.settings.Biome = pickRandom(this.biomeValues).Id;
			this.gameSettingsControl.updateGameAttributes();
			this.gameSettingsControl.pickRandomItems();
		}
	}
};

GameSettingControls.Biome.prototype.TitleCaption =
	translate("Biome");

GameSettingControls.Biome.prototype.RandomBiomeId =
	"random";

GameSettingControls.Biome.prototype.Tooltip =
	translate("Select the flora and fauna.");

GameSettingControls.Biome.prototype.RandomBiome =
	translateWithContext("biome", "Random");

GameSettingControls.Biome.prototype.RandomDescription =
	translate("Pick a biome at random.");

GameSettingControls.Biome.prototype.AutocompleteOrder = 0;
