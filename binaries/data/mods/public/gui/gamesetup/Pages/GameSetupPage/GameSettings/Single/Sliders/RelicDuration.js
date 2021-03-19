GameSettingControls.RelicDuration = class RelicDuration extends GameSettingControlSlider
{
	constructor(...args)
	{
		super(...args);

		this.sprintfValue = {};
		this.available = false;

		g_GameSettings.relic.watch(() => this.render(), ["duration", "available"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setHidden(!g_GameSettings.relic.available);
		this.setEnabled(g_GameSettings.map.type != "scenario");

		if (g_GameSettings.relic.available)
		{
			let value = g_GameSettings.relic.duration;
			this.sprintfValue.min = value;
			this.setSelectedValue(
				g_GameSettings.relic.duration,
				value == 0 ? this.InstantVictory : sprintf(this.CaptionVictoryTime(value), this.sprintfValue));
		}
	}

	onValueChange(value)
	{
		g_GameSettings.relic.setDuration(value);
		this.gameSettingsControl.setNetworkGameAttributes();
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
