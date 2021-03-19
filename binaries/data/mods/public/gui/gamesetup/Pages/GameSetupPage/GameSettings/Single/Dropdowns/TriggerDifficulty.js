GameSettingControls.TriggerDifficulty = class TriggerDifficulty extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;
		g_GameSettings.triggerDifficulty.watch(() => this.render(), ["value", "available"]);
		this.render();
	}

	onHoverChange()
	{
		this.dropdown.tooltip =
			this.values && this.values.Tooltip[this.dropdown.hovered] ||
			this.Tooltip;
	}

	render()
	{
		this.setHidden(!g_GameSettings.triggerDifficulty.available);
		if (!g_GameSettings.triggerDifficulty.available)
			return;

		this.values = prepareForDropdown(g_GameSettings.triggerDifficulty.getAvailableSettings());

		this.dropdown.list = this.values.Title;
		this.dropdown.list_data = this.values.Difficulty;

		this.setSelectedValue(g_GameSettings.triggerDifficulty.value);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.triggerDifficulty.setValue(this.values.Difficulty[itemIdx]);
		this.gameSettingsControl.setNetworkInitAttributes();
	}
};

GameSettingControls.TriggerDifficulty.prototype.TitleCaption =
	translate("Difficulty");

GameSettingControls.TriggerDifficulty.prototype.Tooltip =
	translate("Select the difficulty of this scenario.");
