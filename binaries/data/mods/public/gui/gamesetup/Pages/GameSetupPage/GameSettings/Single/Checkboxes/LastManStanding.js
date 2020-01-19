GameSettingControls.LastManStanding = class extends GameSettingControlCheckbox
{
	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			!mapData.settings.LockTeams &&
			mapData.settings.LastManStanding;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.LastManStanding)
		{
			g_GameAttributes.settings.LastManStanding = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		this.available = !g_GameAttributes.settings.LockTeams;
		if (this.available)
		{
			if (g_GameAttributes.settings.LastManStanding === undefined)
			{
				g_GameAttributes.settings.LastManStanding = false;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.LastManStanding !== undefined)
		{
			delete g_GameAttributes.settings.LastManStanding;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		// Always display this, so that players are aware that there is this gamemode
		this.setChecked(!!g_GameAttributes.settings.LastManStanding);
		this.setEnabled(g_GameAttributes.mapType != "scenario" && !g_GameAttributes.settings.LockTeams);
	}

	onPress(checked)
	{
		g_GameAttributes.settings.LastManStanding = checked;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.LastManStanding.prototype.TitleCaption =
	translate("Last Man Standing");

GameSettingControls.LastManStanding.prototype.Tooltip =
	translate("Toggle whether the last remaining player or the last remaining set of allies wins.");
