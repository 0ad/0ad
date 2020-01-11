GameSettingControls.ExploredMap = class extends GameSettingControlCheckbox
{
	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.ExploreMap || undefined;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.ExploreMap)
		{
			g_GameAttributes.settings.ExploreMap = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		if (g_GameAttributes.settings.ExploreMap === undefined)
		{
			g_GameAttributes.settings.ExploreMap = !!g_GameAttributes.settings.RevealMap;
			this.gameSettingsControl.updateGameAttributes();
		}
		else if (g_GameAttributes.settings.RevealMap &&
		    !g_GameAttributes.settings.ExploreMap)
		{
			g_GameAttributes.settings.ExploreMap = true;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		this.setChecked(g_GameAttributes.settings.ExploreMap);
		this.setEnabled(g_GameAttributes.mapType != "scenario" && !g_GameAttributes.settings.RevealMap);
	}

	onPress(checked)
	{
		g_GameAttributes.settings.ExploreMap = checked;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.ExploredMap.prototype.TitleCaption =
	// Translation: Make sure to differentiate between the revealed map and explored map settings!
	translate("Explored Map");

GameSettingControls.ExploredMap.prototype.Tooltip =
	// Translation: Make sure to differentiate between the revealed map and explored map settings!
	translate("Toggle explored map (see initial map).");
