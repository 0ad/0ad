GameSettingControls.Nomad = class extends GameSettingControlCheckbox
{
	onMapChange(mapData)
	{
		let available = g_GameAttributes.mapType == "random";
		this.setHidden(!available);
		if (!available)
			return;

		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.Nomad;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.Nomad)
		{
			g_GameAttributes.settings.Nomad = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		if (g_GameAttributes.mapType == "random")
		{
			if (g_GameAttributes.settings.Nomad === undefined)
			{
				g_GameAttributes.settings.Nomad = false;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.Nomad !== undefined)
		{
			delete g_GameAttributes.settings.Nomad;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (!g_GameAttributes.mapType)
			return;

		if (g_GameAttributes.mapType == "random")
			this.setChecked(g_GameAttributes.settings.Nomad);
	}

	onPress(checked)
	{
		g_GameAttributes.settings.Nomad = checked;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.Nomad.prototype.TitleCaption =
	translate("Nomad");

GameSettingControls.Nomad.prototype.Tooltip =
	translate("In Nomad mode, players start with only few units and have to find a suitable place to build their city. Ceasefire is recommended.");
