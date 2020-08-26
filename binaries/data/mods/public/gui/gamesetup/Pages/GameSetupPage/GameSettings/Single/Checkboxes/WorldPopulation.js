GameSettingControls.WorldPopulation = class extends GameSettingControlCheckbox
{
	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.WorldPopulation || undefined;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.WorldPopulation)
		{
			g_GameAttributes.settings.WorldPopulation = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}

		this.setEnabled(g_GameAttributes.mapType != "scenario");
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		if (g_GameAttributes.settings.WorldPopulation !== undefined)
			return;

		g_GameAttributes.settings.WorldPopulation = false;
		this.gameSettingsControl.updateGameAttributes();
	}

	onGameAttributesBatchChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		this.setChecked(g_GameAttributes.settings.WorldPopulation);
	}

	onPress(checked)
	{
		g_GameAttributes.settings.WorldPopulation = checked;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.WorldPopulation.prototype.TitleCaption =
	translate("World population");

GameSettingControls.WorldPopulation.prototype.Tooltip =
	translate("When checked the Population Cap will be evenly distributed over all living players.");
