class GameSettingWarning
{
	constructor(setupWindow, cancelButton)
	{
		if (!g_IsNetworked)
			return;

		this.gameSettingWarning = Engine.GetGUIObjectByName("gameSettingWarning");

		setupWindow.controls.gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));
		cancelButton.registerCancelButtonResizeHandler(this.onCancelButtonResize.bind(this));
	}

	onGameAttributesBatchChange()
	{
		let caption =
			g_GameAttributes.settings.CheatsEnabled ?
				this.CheatsEnabled :
			g_GameAttributes.settings.RatingEnabled ?
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
