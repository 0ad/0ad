GameSettingControls.Nomad = class Nomad extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);
		g_GameSettings.nomad.watch(() => this.render(), ["enabled"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setHidden(g_GameSettings.map.type != "random");
		this.setChecked(g_GameSettings.nomad.enabled);
	}

	onPress(checked)
	{
		g_GameSettings.nomad.setEnabled(checked);
		this.gameSettingsControl.setNetworkInitAttributes();
	}
};

GameSettingControls.Nomad.prototype.TitleCaption =
	translate("Nomad");

GameSettingControls.Nomad.prototype.Tooltip =
	translate("In Nomad mode, players start with only few units and have to find a suitable place to build their city. Ceasefire is recommended.");
