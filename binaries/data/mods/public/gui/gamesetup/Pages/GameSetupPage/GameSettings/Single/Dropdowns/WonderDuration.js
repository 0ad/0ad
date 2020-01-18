GameSettingControls.WonderDuration = class extends GameSettingControlDropdown
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
				g_GameAttributes.settings.WonderDuration = this.values.Duration[this.values.Default];
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
			this.setSelectedValue(g_GameAttributes.settings.WonderDuration);
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.WonderDuration = this.values.Duration[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.WonderDuration.prototype.TitleCaption =
	translate("Wonder Duration");

GameSettingControls.WonderDuration.prototype.Tooltip =
	translate("Minutes until the player has achieved Wonder Victory");

GameSettingControls.WonderDuration.prototype.NameWonderVictory =
	"wonder";
