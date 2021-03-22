/**
 * Cheats are always enabled in singleplayer mode, since they are the choice of that one player.
 */
GameSettingControls.Cheats = class Cheats extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.cheats.watch(() => this.render(), ["enabled"]);
		g_GameSettings.rating.watch(() => this.render(), ["enabled"]);
	}

	onLoad()
	{
		g_GameSettings.cheats.setEnabled(!g_IsNetworked);
		this.render();
	}

	render()
	{
		this.setChecked(g_GameSettings.cheats.enabled);
		this.setEnabled(g_IsNetworked && !g_GameSettings.rating.enabled);
	}

	onPress(checked)
	{
		g_GameSettings.cheats.setEnabled(!g_IsNetworked || checked);
		this.gameSettingsController.setNetworkInitAttributes();
	}
};

GameSettingControls.Cheats.prototype.TitleCaption =
	translate("Cheats");

GameSettingControls.Cheats.prototype.Tooltip =
	translate("Toggle the usability of cheats.");
