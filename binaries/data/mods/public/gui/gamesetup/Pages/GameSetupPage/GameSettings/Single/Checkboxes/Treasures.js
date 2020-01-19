GameSettingControls.Treasures = class extends GameSettingControlCheckbox
{
	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.DisableTreasures;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.DisableTreasures)
		{
			g_GameAttributes.settings.DisableTreasures = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}

		this.setEnabled(g_GameAttributes.mapType != "scenario");
	}

	onGameAttributesChange()
	{
		if (g_GameAttributes.settings.DisableTreasures === undefined)
		{
			g_GameAttributes.settings.DisableTreasures = false;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setChecked(g_GameAttributes.settings.DisableTreasures);
	}

	onPress(checked)
	{
		g_GameAttributes.settings.DisableTreasures = checked;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.Treasures.prototype.TitleCaption =
	translate("Disable Treasures");

GameSettingControls.Treasures.prototype.Tooltip =
	translate("Do not add treasures to the map.");
