GameSettingControls.Biome = class Biome extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.biome.watch(() => this.render(), ["biome", "available"]);
		this.render();
	}

	onHoverChange()
	{
		if (!this.dropdown.list_data[this.dropdown.hovered])
			this.dropdown.tooltip = "";
		else if (this.dropdown.list_data[this.dropdown.hovered] == "random")
			this.dropdown.tooltip = this.RandomDescription;
		else
			this.dropdown.tooltip = g_GameSettings.biome.biomeData[
				this.dropdown.list_data[this.dropdown.hovered]
			].Description;
	}

	render()
	{
		this.setHidden(!g_GameSettings.biome.available.size);

		let values = prepareForDropdown([
			{
				"Title": setStringTags(this.RandomBiome, this.RandomItemTags),
				"Id": "random"
			},
			...g_GameSettings.biome.getAvailableBiomeData()
		]);

		this.dropdown.list = values.Title;
		this.dropdown.list_data = values.Id;

		this.setSelectedValue(g_GameSettings.biome.biome);
	}

	getAutocompleteEntries()
	{
		return g_GameSettings.biome.biomes;
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.biome.setBiome(this.dropdown.list_data[itemIdx]);
		this.gameSettingsController.setNetworkInitAttributes();
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
