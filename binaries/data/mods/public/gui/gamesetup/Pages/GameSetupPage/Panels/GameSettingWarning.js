class GameSettingWarning
{
	constructor(setupWindow, cancelButton)
	{
		if (!g_IsNetworked)
			return;

		this.gameSettingWarning = Engine.GetGUIObjectByName("gameSettingWarning");

		g_GameSettings.cheats.watch(() => this.onSettingsChange(), ["enabled"]);
		g_GameSettings.rating.watch(() => this.onSettingsChange(), ["enabled"]);
	}

	onSettingsChange()
	{
		let caption =
			g_GameSettings.cheats.enabled ?
				this.CheatsEnabled :
				g_GameSettings.rating.enabled ?
					this.RatingEnabled :
					"";

		this.gameSettingWarning.caption = caption;
		this.gameSettingWarning.hidden = !caption;
	}
}

GameSettingWarning.prototype.CheatsEnabled =
	translate("Cheats enabled.");

GameSettingWarning.prototype.RatingEnabled =
	translate("Rated game.");
