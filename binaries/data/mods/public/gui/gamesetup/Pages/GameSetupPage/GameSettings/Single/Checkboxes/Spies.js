GameSettingControls.Spies = class Spies extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.disableSpies.watch(() => this.render(), ["enabled"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario");
		this.setChecked(g_GameSettings.disableSpies.enabled);
	}

	onPress(checked)
	{
		g_GameSettings.disableSpies.setEnabled(checked);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.Spies.prototype.TitleCaption =
	translate("Disable Spies");

GameSettingControls.Spies.prototype.Tooltip =
	translate("Disable spies during the game.");
