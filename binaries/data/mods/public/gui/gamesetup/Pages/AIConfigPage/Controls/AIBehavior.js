AIGameSettingControls.AIBehavior = class extends AIGameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.fixedAIBehavior = [];
		this.defaultBehavior = Engine.ConfigDB_GetValue("user", this.ConfigBehavior);

		this.dropdown.list = g_Settings.AIBehaviors.map(AIBehavior => AIBehavior.Title);
		this.dropdown.list_data = g_Settings.AIBehaviors.map(AIBehavior => AIBehavior.Name);
	}

	onAssignPlayer(source, target)
	{
		if (source && target.AIBehavior)
			source.AIBehavior = target.AIBehavior;

		delete target.AIBehavior;
	}

	onMapChange(mapData)
	{
		for (let playerIndex = 0; playerIndex < g_MaxPlayers; ++playerIndex)
		{
			let mapPData = this.gameSettingsControl.getPlayerData(mapData, playerIndex);
			this.fixedAIBehavior[playerIndex] =
				mapPData && mapPData.AI ?
					(mapPData.AIBehavior !== undefined ?
						mapPData.AIBehavior :
						g_Settings.PlayerDefaults[this.playerIndex + 1].AIBehavior) :
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
			if (this.fixedAIBehavior[playerIndex] && pData.AIBehavior !== this.fixedAIBehavior[playerIndex])
			{
				pData.AIBehavior = this.fixedAIBehavior[playerIndex];
				this.gameSettingsControl.updateGameAttributes();
			}
			else if (pData.AIDiff !== undefined &&
				g_Settings.AIDifficulties[pData.AIDiff].Name == "sandbox" &&
				pData.AIBehavior != "balanced")
			{
				pData.AIBehavior = "balanced";
				this.gameSettingsControl.updateGameAttributes();
			}
			else if (pData.AIBehavior === undefined)
			{
				pData.AIBehavior = this.defaultBehavior;
				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (pData.AIBehavior !== undefined)
		{
			delete pData.AIBehavior;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	updateSelectedValue()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		this.setHidden(!pData || !pData.AI || g_Settings.AIDifficulties[pData.AIDiff].Name == "sandbox");
		if (pData && pData.AI && pData.AIDiff !== undefined && pData.AIBehavior !== undefined)
			this.setSelectedValue(pData.AIBehavior);
	}

	onSelectionChange(itemIdx)
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!g_IsController || !pData)
			return;

		pData.AIBehavior = g_Settings.AIBehaviors[itemIdx].Name;
		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
}

AIGameSettingControls.AIBehavior.prototype.ConfigBehavior =
	"gui.gamesetup.aibehavior";

AIGameSettingControls.AIBehavior.prototype.TitleCaption =
	translate("AI Behavior");
