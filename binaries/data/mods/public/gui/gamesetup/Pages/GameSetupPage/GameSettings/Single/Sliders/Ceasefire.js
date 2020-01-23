GameSettingControls.Ceasefire = class extends GameSettingControlSlider
{
	constructor(...args)
	{
		super(...args);

		this.sprintfValue = {};
	}

	onMapChange(mapData)
	{
		let mapValue =
			mapData &&
			mapData.settings &&
			mapData.settings.Ceasefire || undefined;

		if (mapValue !== undefined && mapValue != g_GameAttributes.settings.Ceasefire)
		{
			g_GameAttributes.settings.Ceasefire = mapValue;
			this.gameSettingsControl.updateGameAttributes();
		}

		this.setEnabled(g_GameAttributes.mapType != "scenario");
	}

	onGameAttributesChange()
	{
		if (g_GameAttributes.settings.Ceasefire == undefined)
		{
			g_GameAttributes.settings.Ceasefire = this.DefaultValue;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		let value = Math.round(g_GameAttributes.settings.Ceasefire);
		this.sprintfValue.minutes = value;

		this.setSelectedValue(
			g_GameAttributes.settings.Ceasefire,
			value == 0 ?
				this.NoCeasefireCaption :
				sprintf(this.CeasefireCaption(value), this.sprintfValue));
	}

	onValueChange(value)
	{
		g_GameAttributes.settings.Ceasefire = value;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onGameAttributesFinalize()
	{
		g_GameAttributes.settings.Ceasefire = Math.round(g_GameAttributes.settings.Ceasefire);
	}
};

GameSettingControls.Ceasefire.prototype.TitleCaption =
	translate("Ceasefire");

GameSettingControls.Ceasefire.prototype.Tooltip =
	translate("Set time where no attacks are possible.");

GameSettingControls.Ceasefire.prototype.NoCeasefireCaption =
	translateWithContext("ceasefire", "No ceasefire");

GameSettingControls.Ceasefire.prototype.CeasefireCaption =
	minutes => translatePluralWithContext("ceasefire", "%(minutes)s minute", "%(minutes)s minutes", minutes);

GameSettingControls.Ceasefire.prototype.DefaultValue = 0;

GameSettingControls.Ceasefire.prototype.MinValue = 0;

GameSettingControls.Ceasefire.prototype.MaxValue = 45;
