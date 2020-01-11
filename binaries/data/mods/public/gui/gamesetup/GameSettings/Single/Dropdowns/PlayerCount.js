GameSettingControls.PlayerCount = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = Array.from(
			new Array(g_MaxPlayers),
			(v, i) => i + 1);

		this.dropdown.list = this.values;
		this.dropdown.list_data = this.values;
	}

	onMapChange(mapData)
	{
		if (mapData &&
			mapData.settings &&
			mapData.settings.PlayerData &&
			mapData.settings.PlayerData.length != g_GameAttributes.settings.PlayerData.length)
		{
			this.onSelectionChange(this.values.indexOf(mapData.settings.PlayerData.length));
		}

		this.setEnabled(g_GameAttributes.mapType == "random");
	}

	onGameAttributesBatchChange()
	{
		this.setSelectedValue(g_GameAttributes.settings.PlayerData.length);
	}

	onSelectionChange(itemIdx)
	{
		let length = this.values[itemIdx];
		if (g_GameAttributes.settings.PlayerData.length == length)
			return;

		g_GameAttributes.settings.PlayerData.length = length;

		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();

		this.playerAssignmentsControl.unassignInvalidPlayers();
	}
};

GameSettingControls.PlayerCount.prototype.TitleCaption =
	translate("Number of Players");

GameSettingControls.PlayerCount.prototype.Tooltip =
	translate("Select number of players.");
