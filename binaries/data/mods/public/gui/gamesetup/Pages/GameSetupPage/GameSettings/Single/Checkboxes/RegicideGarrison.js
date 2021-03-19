GameSettingControls.RegicideGarrison = class RegicideGarrison extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.regicideGarrison.watch(() => this.render(), ["enabled", "available"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setHidden(g_GameSettings.map.type == "scenario" ||
			!g_GameSettings.regicideGarrison.available);
		this.setChecked(g_GameSettings.regicideGarrison.enabled);
	}

	onPress(checked)
	{
		g_GameSettings.regicideGarrison.setEnabled(checked);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.RegicideGarrison.prototype.TitleCaption =
	translate("Hero Garrison");

GameSettingControls.RegicideGarrison.prototype.Tooltip =
	translate("Toggle whether heroes can be garrisoned.");
