GameSettingControls.Treasures = class Treasures extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);
		g_GameSettings.disableTreasures.watch(() => this.render(), ["enabled"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario");
		this.setChecked(g_GameSettings.disableTreasures.enabled);
	}

	onPress(checked)
	{
		g_GameSettings.disableTreasures.setEnabled(checked);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.Treasures.prototype.TitleCaption =
	translate("Disable Treasures");

GameSettingControls.Treasures.prototype.Tooltip =
	translate("Do not add treasures to the map.");
