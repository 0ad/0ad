GameSettingControls.LastManStanding = class LastManStanding extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);
		g_GameSettings.lastManStanding.watch(() => this.render(), ["enabled", "available"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
	}

	render()
	{
		// Always display this, so that players are aware that there is this gamemode
		this.setChecked(g_GameSettings.lastManStanding.enabled);
		this.setEnabled(g_GameSettings.map.type != "scenario" &&
			g_GameSettings.lastManStanding.available);
	}

	onPress(checked)
	{
		g_GameSettings.lastManStanding.setEnabled(checked);
		this.gameSettingsController.setNetworkInitAttributes();
	}
};

GameSettingControls.LastManStanding.prototype.TitleCaption =
	translate("Last Man Standing");

GameSettingControls.LastManStanding.prototype.Tooltip =
	translate("Toggle whether the last remaining player or the last remaining set of allies wins.");
