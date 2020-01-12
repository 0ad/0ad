GameSettingControls.RegicideGarrison = class extends GameSettingControlCheckbox
{
	onMapChange(mapData)
	{
		this.setEnabled(g_GameAttributes.mapType != "scenario");

		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.VictoryConditions &&
			mapData.settings.VictoryConditions.indexOf(this.RegicideName) != -1 &&
			mapData.settings.RegicideGarrison;

		if (mapValue !== undefined || !g_GameAttributes.settings || mapValue == g_GameAttributes.settings.RegicideGarrison)
			return;

		if (!g_GameAttributes.settings.VictoryConditions)
			g_GameAttributes.settings.VictoryConditions = [];

		if (g_GameAttributes.settings.VictoryConditions.indexOf(this.RegicideName) == -1)
			g_GameAttributes.settings.VictoryConditions.push(this.RegicideName);

		g_GameAttributes.settings.RegicideGarrison = mapValue;

		this.gameSettingsControl.updateGameAttributes();
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.settings.VictoryConditions)
			return;

		let available = g_GameAttributes.settings.VictoryConditions.indexOf(this.RegicideName) != -1;
		this.setHidden(!available);

		if (available)
		{
			if (g_GameAttributes.settings.RegicideGarrison === undefined)
			{
				g_GameAttributes.settings.RegicideGarrison = false;
				this.gameSettingsControl.updateGameAttributes();
			}
			this.setChecked(g_GameAttributes.settings.RegicideGarrison);
		}
		else if (g_GameAttributes.settings.RegicideGarrison !== undefined)
		{
			delete g_GameAttributes.settings.RegicideGarrison;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onPress(checked)
	{
		g_GameAttributes.settings.RegicideGarrison = checked;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.RegicideGarrison.prototype.TitleCaption =
	translate("Hero Garrison");

GameSettingControls.RegicideGarrison.prototype.Tooltip =
	translate("Toggle whether heroes can be garrisoned.");

GameSettingControls.RegicideGarrison.prototype.RegicideName =
	"regicide";
