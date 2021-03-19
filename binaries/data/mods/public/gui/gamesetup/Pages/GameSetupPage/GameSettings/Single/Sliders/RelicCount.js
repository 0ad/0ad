GameSettingControls.RelicCount = class RelicCount extends GameSettingControlSlider
{
	constructor(...args)
	{
		super(...args);

		this.sprintfValue = {};
		this.available = false;

		g_GameSettings.relic.watch(() => this.render(), ["count", "available"]);
		g_GameSettings.map.watch(() => this.render(), ["type"]);
		this.render();
	}

	render()
	{
		this.setHidden(!g_GameSettings.relic.available);
		this.setEnabled(g_GameSettings.map.type != "scenario");

		if (g_GameSettings.relic.available)
		{
			let value = g_GameSettings.relic.count;
			this.sprintfValue.number = value;
			this.setSelectedValue(
				g_GameSettings.relic.count,
				value == 0 ? this.InstantVictory : sprintf(this.CaptionRelicCount(value), this.sprintfValue));
		}
	}

	onValueChange(value)
	{
		g_GameSettings.relic.setCount(value);
		this.gameSettingsControl.setNetworkInitAttributes();
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
