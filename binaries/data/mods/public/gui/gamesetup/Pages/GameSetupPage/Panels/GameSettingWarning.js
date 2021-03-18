class GameSettingWarning
{
	constructor(setupWindow, cancelButton)
	{
		if (!g_IsNetworked)
			return;

		this.gameSettingWarning = Engine.GetGUIObjectByName("gameSettingWarning");

		cancelButton.registerCancelButtonResizeHandler(this.onCancelButtonResize.bind(this));

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

	onCancelButtonResize(cancelButton)
	{
		let size = this.gameSettingWarning.size;
		size.right = cancelButton.size.left - this.Margin;
		this.gameSettingWarning.size = size;
	}
}

GameSettingWarning.prototype.Margin = 10;

GameSettingWarning.prototype.CheatsEnabled =
	translate("Cheats enabled.");

GameSettingWarning.prototype.RatingEnabled =
	translate("Rated game.");
