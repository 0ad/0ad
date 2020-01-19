GameSettingControls.RelicDuration = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = prepareForDropdown(g_Settings.VictoryDurations);

		this.dropdown.list = this.values.Title;
		this.dropdown.list_data = this.values.Duration;

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
				g_GameAttributes.settings.RelicDuration = this.values.Duration[this.values.Default];
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
			this.setSelectedValue(g_GameAttributes.settings.RelicDuration);
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.RelicDuration = this.values.Duration[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.RelicDuration.prototype.TitleCaption =
	translate("Relic Duration");

GameSettingControls.RelicDuration.prototype.Tooltip =
	translate("Minutes until the player has achieved Relic Victory.");

GameSettingControls.RelicDuration.prototype.NameCaptureTheRelic =
	"capture_the_relic";
