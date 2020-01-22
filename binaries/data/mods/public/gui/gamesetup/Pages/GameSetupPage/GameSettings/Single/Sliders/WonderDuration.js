GameSettingControls.WonderDuration = class extends GameSettingControlSlider
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
			mapData.settings.VictoryConditions.indexOf(this.NameWonderVictory) != -1 &&
			mapData.settings.WonderDuration || undefined;

		if (mapValue === undefined || mapValue == g_GameAttributes.settings.WonderDuration)
			return;

		if (!g_GameAttributes.settings.VictoryConditions)
			g_GameAttributes.settings.VictoryConditions = [];

		if (g_GameAttributes.settings.VictoryConditions.indexOf(this.NameWonderVictory) == -1)
			g_GameAttributes.settings.VictoryConditions.push(this.NameWonderVictory);

		g_GameAttributes.settings.WonderDuration = mapValue;

		this.gameSettingsControl.updateGameAttributes();
	}

	onGameAttributesChange()
	{
		if (!g_GameAttributes.settings.VictoryConditions)
			return;

		this.available = g_GameAttributes.settings.VictoryConditions.indexOf(this.NameWonderVictory) != -1;

		if (this.available)
		{
			if (g_GameAttributes.settings.WonderDuration === undefined)
			{
				g_GameAttributes.settings.WonderDuration = this.DefaultValue;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (g_GameAttributes.settings.WonderDuration !== undefined)
		{
			delete g_GameAttributes.settings.WonderDuration;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setHidden(!this.available);

		if (this.available)
		{
			let value = Math.round(g_GameAttributes.settings.WonderDuration);
			this.sprintfValue.min = value;
			this.setSelectedValue(
				g_GameAttributes.settings.WonderDuration,
				value == 0 ? this.InstantVictory : sprintf(this.CaptionVictoryTime(value), this.sprintfValue));
		}
	}

	onValueChange(value)
	{
		g_GameAttributes.settings.WonderDuration = value;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}

	onGameAttributesFinalize()
	{
		if (this.available)
			g_GameAttributes.settings.WonderDuration = Math.round(g_GameAttributes.settings.WonderDuration);
	}
};

GameSettingControls.WonderDuration.prototype.TitleCaption =
	translate("Wonder Duration");

GameSettingControls.WonderDuration.prototype.Tooltip =
	translate("Minutes until the player has achieved Wonder Victory");

GameSettingControls.WonderDuration.prototype.NameWonderVictory =
	"wonder";

GameSettingControls.WonderDuration.prototype.CaptionVictoryTime =
	min => translatePluralWithContext("victory duration", "%(min)s minute", "%(min)s minutes", min);

GameSettingControls.WonderDuration.prototype.InstantVictory =
	translateWithContext("victory duration", "Immediate Victory.");

GameSettingControls.WonderDuration.prototype.MinValue = 0;

GameSettingControls.WonderDuration.prototype.MaxValue = 60;

GameSettingControls.WonderDuration.prototype.DefaultValue = 20;
