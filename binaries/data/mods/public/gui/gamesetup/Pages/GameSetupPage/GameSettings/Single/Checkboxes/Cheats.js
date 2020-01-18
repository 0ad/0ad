/**
 * Cheats are always enabled in singleplayer mode, since they are the choice of that one player.
 */
GameSettingControls.Cheats = class extends GameSettingControlCheckbox
{
	constructor(...args)
	{
		super(...args);
		this.setHidden(!g_IsNetworked);
	}

	onGameAttributesChange()
	{
		if (g_GameAttributes.settings.CheatsEnabled === undefined ||
			g_GameAttributes.settings.CheatsEnabled && g_GameAttributes.settings.RatingEnabled ||
			!g_GameAttributes.settings.CheatsEnabled && !g_IsNetworked)
		{
			g_GameAttributes.settings.CheatsEnabled = !g_IsNetworked;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setChecked(g_GameAttributes.settings.CheatsEnabled);
		this.setEnabled(!g_GameAttributes.settings.RatingEnabled);
	}

	onPress(checked)
	{
		g_GameAttributes.settings.CheatsEnabled =
			!g_IsNetworked ||
			checked && !g_GameAttributes.settings.RatingEnabled;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.Cheats.prototype.TitleCaption =
	translate("Cheats");

GameSettingControls.Cheats.prototype.Tooltip =
	translate("Toggle the usability of cheats.");
