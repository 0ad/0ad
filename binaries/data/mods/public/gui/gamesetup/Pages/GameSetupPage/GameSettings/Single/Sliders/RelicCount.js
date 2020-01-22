GameSettingControls.RelicCount = class extends GameSettingControlSlider
{
	constructor(...args)
	{
		super(...args);

		this.sprintfValue = {};
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
				g_GameAttributes.settings.RelicCount = this.DefaultValue;
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
		{
			let value = Math.round(g_GameAttributes.settings.RelicCount);
			this.sprintfValue.number = value;
			this.setSelectedValue(
				g_GameAttributes.settings.RelicCount,
				value == 0 ? this.InstantVictory : sprintf(this.CaptionRelicCount(value), this.sprintfValue));
		}
	}

	onValueChange(value)
	{
		g_GameAttributes.settings.RelicCount = value;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onGameAttributesFinalize()
	{
		if (this.available)
			g_GameAttributes.settings.RelicCount = Math.round(g_GameAttributes.settings.RelicCount);
	}
};

GameSettingControls.RelicCount.prototype.TitleCaption =
	translate("Relic Count");

GameSettingControls.RelicCount.prototype.CaptionRelicCount =
	relicCount => translatePlural("%(number)s relic", "%(number)s relics", relicCount);

GameSettingControls.RelicCount.prototype.Tooltip =
	translate("Total number of relics spawned on the map. Relic victory is most realistic with only one or two relics. With greater numbers, the relics are important to capture to receive aura bonuses.");

GameSettingControls.RelicCount.prototype.NameCaptureTheRelic =
	"capture_the_relic";

GameSettingControls.RelicCount.prototype.MinValue = 1;

GameSettingControls.RelicCount.prototype.MaxValue = Object.keys(g_CivData).length;

GameSettingControls.RelicCount.prototype.DefaultValue = 2;
