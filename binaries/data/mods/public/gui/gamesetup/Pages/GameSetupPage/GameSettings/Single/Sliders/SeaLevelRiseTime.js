GameSettingControls.SeaLevelRiseTime = class extends GameSettingControlSlider
{
	constructor(...args)
	{
		super(...args);

		this.values = undefined;
		this.sprintfValue = {};
	}

	onMapChange(mapData)
	{
		this.values =
			mapData &&
			mapData.settings &&
			mapData.settings.SeaLevelRise || undefined;

		if (this.values)
		{
			this.slider.min_value = this.values.Min;
			this.slider.max_value = this.values.Max;
		}

		this.setHidden(!this.values);
		this.setEnabled(g_GameAttributes.mapType != "scenario");
	}

	onGameAttributesChange()
	{
		if (this.values)
		{
			if (g_GameAttributes.settings.SeaLevelRiseTime === undefined)
			{
				g_GameAttributes.settings.SeaLevelRiseTime = this.values.Default;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.SeaLevelRiseTime !== undefined)
		{
			delete g_GameAttributes.settings.SeaLevelRiseTime;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		if (!this.values)
			return;

		let value = Math.round(g_GameAttributes.settings.SeaLevelRiseTime);
		this.sprintfValue.minutes = value;

		this.setSelectedValue(
			g_GameAttributes.settings.SeaLevelRiseTime,
			sprintf(this.SeaLevelRiseTimeCaption(value), this.sprintfValue));
	}

	onValueChange(value)
	{
		g_GameAttributes.settings.SeaLevelRiseTime = value;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onGameAttributesFinalize()
	{
		if (this.values)
			g_GameAttributes.settings.SeaLevelRiseTime = Math.round(g_GameAttributes.settings.SeaLevelRiseTime);
	}
};

GameSettingControls.SeaLevelRiseTime.prototype.TitleCaption =
	translate("Sea Level Rise Time");

GameSettingControls.SeaLevelRiseTime.prototype.Tooltip =
	translate("Set the time when the water will start to rise.");

GameSettingControls.SeaLevelRiseTime.prototype.SeaLevelRiseTimeCaption =
	minutes => translatePluralWithContext("sea level rise time", "%(minutes)s minute", "%(minutes)s minutes", minutes);

GameSettingControls.SeaLevelRiseTime.prototype.MinValue = 0;

GameSettingControls.SeaLevelRiseTime.prototype.MaxValue = 60;
