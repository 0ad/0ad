GameSettingControls.WaterLevel = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.waterLevel.watch(() => this.render(), ["value", "options"]);
		this.render();
	}

	onHoverChange()
	{
		this.dropdown.tooltip = this.values?.Description[this.dropdown.hovered] ?? this.Tooltip;
	}

	render()
	{
		this.setHidden(!g_GameSettings.waterLevel.options);
		if (!g_GameSettings.waterLevel.options)
			return;

		this.values = prepareForDropdown(g_GameSettings.waterLevel.options.map(item =>
			({
				"Name": item.Name,
				"Title": translate(item.Name),
				"Description": translate(item.Description),
			})));

		this.dropdown.list = this.values.Title;
		this.dropdown.list_data = this.values.Name;

		this.setSelectedValue(g_GameSettings.waterLevel.value);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.waterLevel.value = this.values.Name[itemIdx];
		this.gameSettingsController.setNetworkInitAttributes();
	}
};

GameSettingControls.WaterLevel.prototype.TitleCaption =
	translate("Water level");

GameSettingControls.WaterLevel.prototype.Tooltip =
	translate("Select the water level of this map.");
