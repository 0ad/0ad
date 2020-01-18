PlayerSettingControls.PlayerTeam = class extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.values = prepareForDropdown([
			{
				"label": this.NoTeam,
				"id": this.NoTeamId
			},
			...Array.from(
				new Array(g_MaxTeams),
				(v, i) => ({
					"label": i + 1,
					"id": i
				}))
		]);
		this.dropdown.list = this.values.label;
		this.dropdown.list_data = this.values.id;
	}

	setControl()
	{
		this.label = Engine.GetGUIObjectByName("playerTeamText[" + this.playerIndex + "]");
		this.dropdown = Engine.GetGUIObjectByName("playerTeam[" + this.playerIndex + "]");
	}

	onMapChange(mapData)
	{
		let mapPData = this.gameSettingsControl.getPlayerData(mapData, this.playerIndex);
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);

		if (pData && mapPData && mapPData.Team !== undefined)
		{
			pData.Team = mapPData.Team;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		if (pData.Team === undefined)
		{
			pData.Team = this.NoTeamId;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		this.setEnabled(g_GameAttributes.mapType != "scenario");
		this.setSelectedValue(pData.Team);
	}

	onSelectionChange(itemIdx)
	{
		g_GameAttributes.settings.PlayerData[this.playerIndex].Team = itemIdx - 1;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

PlayerSettingControls.PlayerTeam.prototype.Tooltip =
	translate("Select player's team.");

PlayerSettingControls.PlayerTeam.prototype.NoTeam =
	translateWithContext("team", "None");

PlayerSettingControls.PlayerTeam.prototype.NoTeamId = -1;
