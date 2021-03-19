GameSettingControls.LockedTeams = class LockedTeams extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		g_GameSettings.rating.watch(() => this.render(), ["available", "enabled"]);
		this.render();
	}

	onLoad()
	{
		g_GameSettings.lockedTeams.setEnabled(this.DefaultValue);
	}

	render()
	{
		this.setEnabled(g_GameSettings.map.type != "scenario" && g_GameSettings.lockedTeams.available);
		this.setChecked(g_GameSettings.lockedTeams.enabled);
	}

	onPress(checked)
	{
		g_GameSettings.lockedTeams.setEnabled(checked);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.LockedTeams.prototype.TitleCaption =
	translate("Teams Locked");

GameSettingControls.LockedTeams.prototype.Tooltip =
	translate("Toggle locked teams.");

/**
 * In multiplayer mode, players negotiate teams before starting the match and
 * expect to play the match with these teams unless explicitly stated otherwise during the match settings.
 * For singleplayermode, preserve the historic default of open diplomacies.
 */
GameSettingControls.LockedTeams.prototype.DefaultValue = Engine.HasNetClient();
