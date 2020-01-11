GameSettingControls.Ceasefire = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = prepareForDropdown(g_Settings.Ceasefire);

		this.dropdown.list = this.values.Title;
		this.dropdown.list_data = this.values.Duration;
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
			g_GameAttributes.settings.Ceasefire = this.values.Default;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		this.setSelectedValue(g_GameAttributes.settings.Ceasefire);
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.Ceasefire = this.values.Duration[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

GameSettingControls.Ceasefire.prototype.TitleCaption =
	translate("Ceasefire");

GameSettingControls.Ceasefire.prototype.Tooltip =
	translate("Set time where no attacks are possible.");
