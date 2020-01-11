GameSettingControls.RelicCount = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = Array.from(new Array(g_CivData.length), (v, i) => i + 1);
		this.dropdown.list = this.values;
		this.dropdown.list_data = this.values;

		this.available = false;
	}

	onMapChange(mapData)
	{
		this.setEnabled(g_GameAttributes.mapType != "scenario");

		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.VictoryConditions &&
			mapData.settings.VictoryConditions.indexOf(this.NameCaptureTheRelic) != -1 &&
			mapData.settings.RelicCount || undefined;

		if (mapValue === undefined || mapValue == g_GameAttributes.settings.RelicCount)
			return;

		if (!g_GameAttributes.settings.VictoryConditions)
			g_GameAttributes.settings.VictoryConditions = [];

		if (g_GameAttributes.settings.VictoryConditions.indexOf(this.NameCaptureTheRelic) == -1)
			g_GameAttributes.settings.VictoryConditions.push(this.NameCaptureTheRelic);

		g_GameAttributes.settings.RelicCount = mapValue;
		this.gameSettingsControl.updateGameAttributes();
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.settings.VictoryConditions)
			return;

		this.available = g_GameAttributes.settings.VictoryConditions.indexOf(this.NameCaptureTheRelic) != -1;

		if (this.available)
		{
			if (g_GameAttributes.settings.RelicCount === undefined)
			{
				g_GameAttributes.settings.RelicCount = this.DefaultRelicCount;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.RelicCount !== undefined)
		{
			delete g_GameAttributes.settings.RelicCount;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setHidden(!this.available);

		if (this.available)
			this.setSelectedValue(g_GameAttributes.settings.RelicCount);
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.RelicCount = this.values[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.RelicCount.prototype.TitleCaption =
	translate("Relic Count");

GameSettingControls.RelicCount.prototype.Tooltip =
	translate("Total number of relics spawned on the map. Relic victory is most realistic with only one or two relics. With greater numbers, the relics are important to capture to receive aura bonuses.");

GameSettingControls.RelicCount.prototype.NameCaptureTheRelic =
	this.NameCaptureTheRelic;

GameSettingControls.RelicCount.prototype.DefaultRelicCount = 2;
