AIGameSettingControls.AIDifficulty = class extends AIGameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.fixedAIDiff = [];
		this.defaultAIDiff = +Engine.ConfigDB_GetValue("user", this.ConfigDifficulty);

		this.dropdown.list = g_Settings.AIDifficulties.map(AI => AI.Title);
		this.dropdown.list_data = g_Settings.AIDifficulties.map((AI, i) => i);
	}

	onAssignPlayer(source, target)
	{
		if (source && target.AIDiff !== undefined)
			source.AIDiff = target.AIDiff;

		delete target.AIDiff;
	}

	onMapChange(mapData)
	{
		for (let playerIndex = 0; playerIndex < g_MaxPlayers; ++playerIndex)
		{
			let mapPData = this.gameSettingsControl.getPlayerData(mapData, playerIndex);
			this.fixedAIDiff[playerIndex] =
				mapPData && mapPData.AI ?
					(mapPData.AIDiff !== undefined ?
						mapPData.AIDiff :
						g_Settings.PlayerDefaults[this.playerIndex + 1].AIDiff) :
					undefined;
		}
	}

	onGameAttributesChangePlayer(playerIndex)
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, playerIndex);
		if (!pData)
			return;

		if (pData.AI)
		{
			if (this.fixedAIDiff[playerIndex] !== undefined && pData.AIDiff !== this.fixedAIDiff[playerIndex])
			{
				pData.AIDiff = this.fixedAIDiff[playerIndex];
				this.gameSettingsControl.updateGameAttributes();
			}
			else if (pData.AIDiff === undefined)
			{
				pData.AIDiff = this.defaultAIDiff;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (pData.AIDiff !== undefined)
		{
			delete pData.AIDiff;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	updateSelectedValue()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		this.setHidden(!pData || !pData.AI);

		if (pData && pData.AIDiff !== undefined)
			this.setSelectedValue(pData.AIDiff);
	}

	onSelectionChange(itemIdx)
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!g_IsController || !pData)
			return;

		pData.AIDiff = itemIdx;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

AIGameSettingControls.AIDifficulty.prototype.ConfigDifficulty =
	"gui.gamesetup.aidifficulty";

AIGameSettingControls.AIDifficulty.prototype.TitleCaption =
	translate("AI Difficulty");
