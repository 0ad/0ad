GameSettingControls.RevealedMap = class extends GameSettingControlCheckbox
{
	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.RevealMap || undefined;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.RevealMap)
		{
			g_GameAttributes.settings.RevealMap = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}

		this.setEnabled(g_GameAttributes.mapType != "scenario");
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		if (g_GameAttributes.settings.RevealMap === undefined)
		{
			g_GameAttributes.settings.RevealMap = false;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		this.setChecked(g_GameAttributes.settings.RevealMap);
	}

	onPress(checked)
	{
		g_GameAttributes.settings.RevealMap = checked;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.RevealedMap.prototype.TitleCaption =
	// Translation: Make sure to differentiate between the revealed map and explored map settings!
	translate("Revealed Map");

GameSettingControls.RevealedMap.prototype.Tooltip =
	// Translation: Make sure to differentiate between the revealed map and explored map settings!
	translate("Toggle revealed map (see everything).");
