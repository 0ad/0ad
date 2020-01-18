GameSettingControls.Spies = class extends GameSettingControlCheckbox
{
	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.DisableSpies;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.DisableSpies)
		{
			g_GameAttributes.settings.DisableSpies = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}

		this.setEnabled(g_GameAttributes.mapType != "scenario");
	}

	onGameAttributesChange()
	{
		if (g_GameAttributes.settings.DisableSpies === undefined)
		{
			g_GameAttributes.settings.DisableSpies = false;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setChecked(g_GameAttributes.settings.DisableSpies);
	}

	onPress(checked)
	{
		g_GameAttributes.settings.DisableSpies = checked;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.Spies.prototype.TitleCaption =
	translate("Disable Spies");

GameSettingControls.Spies.prototype.Tooltip =
	translate("Disable spies during the game.");
