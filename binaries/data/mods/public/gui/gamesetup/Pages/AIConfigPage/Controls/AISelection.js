AIGameSettingControls.AISelection = class extends AIGameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.fixedAI = [];

		this.values = prepareForDropdown([
			this.NoAI,
			...g_Settings.AIDescriptions.map(AI => ({
				"Title": AI.data.name,
				"Id": AI.id
			}))
		]);

		this.dropdown.list = this.values.Title;
		this.dropdown.list_data = this.values.Id.map((v, i) => i);
	}

	onAssignPlayer(source, target)
	{
		if (source && target.AI)
			source.AI = target.AI;

		target.AI = false;
	}

	onMapChange(mapData)
	{
		for (let playerIndex = 0; playerIndex < g_MaxPlayers; ++playerIndex)
		{
			let mapPData = this.gameSettingsControl.getPlayerData(mapData, playerIndex);
			this.fixedAI[playerIndex] = mapPData && mapPData.AI || undefined;
		}
	}

	onGameAttributesChangePlayer(playerIndex)
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, playerIndex);
		if (!pData)
			return;

		if (this.fixedAI[playerIndex] && pData.AI !== this.fixedAI[playerIndex])
		{
			pData.AI = this.fixedAI[playerIndex];
			this.gameSettingsControl.updateGameAttributes();
		}
		else if (pData.AI === undefined)
		{
			let assignedGUID;
			for (let guid in g_PlayerAssignments)
				if (g_PlayerAssignments[guid].player == playerIndex + 1)
				{
					assignedGUID = guid;
					break;
				}

			pData.AI = assignedGUID ? false : g_Settings.PlayerDefaults[playerIndex + 1].AI;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	updateSelectedValue()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData || pData.AI === undefined)
			return;

		this.setSelectedValue(this.values.Id.indexOf(pData.AI));
	}

	onSelectionChange(itemIdx)
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		pData.AI = this.values.Id[itemIdx];
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
}

AIGameSettingControls.AISelection.prototype.NoAI = {
	"Title": translateWithContext("ai", "None"),
	"Id": false
};

AIGameSettingControls.AISelection.prototype.TitleCaption =
	translate("AI Player");
