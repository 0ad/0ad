GameSettingControls.RelicDuration = class extends GameSettingControlSlider
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
			mapData.settings.RelicDuration || undefined;

		if (mapValue === undefined || mapValue == g_GameAttributes.settings.RelicDuration)
			return;

		if (!g_GameAttributes.settings.VictoryConditions)
			g_GameAttributes.settings.VictoryConditions = [];

		if (g_GameAttributes.settings.VictoryConditions.indexOf(this.NameCaptureTheRelic) == -1)
			g_GameAttributes.settings.VictoryConditions.push(this.NameCaptureTheRelic);

		g_GameAttributes.settings.RelicDuration = mapValue;
		this.gameSettingsControl.updateGameAttributes();
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.settings.VictoryConditions)
			return;

		this.available = g_GameAttributes.settings.VictoryConditions.indexOf(this.NameCaptureTheRelic) != -1;

		if (this.available)
		{
			if (g_GameAttributes.settings.RelicDuration === undefined)
			{
				g_GameAttributes.settings.RelicDuration = this.DefaultValue;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.RelicDuration !== undefined)
		{
			delete g_GameAttributes.settings.RelicDuration;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setHidden(!this.available);

		if (this.available)
		{
			let value = Math.round(g_GameAttributes.settings.RelicDuration);
			this.sprintfValue.min = value;
			this.setSelectedValue(
				g_GameAttributes.settings.RelicDuration,
				value == 0 ? this.InstantVictory : sprintf(this.CaptionVictoryTime(value), this.sprintfValue));
		}
	}

	onValueChange(value)
	{
		g_GameAttributes.settings.RelicDuration = value;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onGameAttributesFinalize()
	{
		if (this.available)
			g_GameAttributes.settings.RelicDuration = Math.round(g_GameAttributes.settings.RelicDuration);
	}
};

GameSettingControls.RelicDuration.prototype.TitleCaption =
	translate("Relic Duration");

GameSettingControls.RelicDuration.prototype.Tooltip =
	translate("Minutes until the player has achieved Relic Victory.");

GameSettingControls.RelicDuration.prototype.NameCaptureTheRelic =
	"capture_the_relic";

GameSettingControls.RelicDuration.prototype.CaptionVictoryTime =
	min => translatePluralWithContext("victory duration", "%(min)s minute", "%(min)s minutes", min);

GameSettingControls.RelicDuration.prototype.InstantVictory =
	translateWithContext("victory duration", "Immediate Victory.");

GameSettingControls.RelicDuration.prototype.MinValue = 0;

GameSettingControls.RelicDuration.prototype.MaxValue = 60;

GameSettingControls.RelicDuration.prototype.DefaultValue = 20;
