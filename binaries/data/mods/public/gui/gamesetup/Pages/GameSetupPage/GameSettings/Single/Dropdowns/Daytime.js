GameSettingControls.Daytime = class Daytime extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;
		g_GameSettings.daytime.watch(() => this.render(), ["value", "data"]);
		this.render();
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values.Description[this.dropdown.hovered] || this.Tooltip;
	}

	render()
	{
		this.setHidden(!g_GameSettings.daytime.data);
		if (!g_GameSettings.daytime.data)
			return;

		this.values = prepareForDropdown([
			{
				"Id": "random",
				"Name": setStringTags(this.RandomTitle, this.RandomItemTags),
				"Description": this.RandomDescription,
				"Preview": g_GameSettings.map.data.settings.Preview
			},
			...g_GameSettings.daytime.data.map(item => ({
				"Id": item.Id,
				"Name": translate(item.Name),
				"Description": translate(item.Description),
				"Preview": item.Preview
			}))
		]);

		this.dropdown.list = this.values.Name;
		this.dropdown.list_data = this.values.Id;

		this.setSelectedValue(g_GameSettings.daytime.value);
	}

	getAutocompleteEntries()
	{
		return this.values && this.values.Name.slice(1);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.daytime.setValue(this.values.Id[itemIdx]);
		this.gameSettingsController.setNetworkInitAttributes();
	}
};

GameSettingControls.Daytime.prototype.TitleCaption =
	translate("Daytime");

GameSettingControls.Daytime.prototype.Tooltip =
	translate("Select whether the match takes place at daylight or night.");

GameSettingControls.Daytime.prototype.RandomTitle =
	translateWithContext("daytime selection", "Random");

GameSettingControls.Daytime.prototype.RandomDescription =
	translateWithContext("daytime selection", "Randomly pick a time of the day.");

GameSettingControls.Daytime.prototype.AutocompleteOrder = 0;
