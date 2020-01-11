PlayerSettingControls.AIConfigButton = class extends GameSettingControl
{
	constructor(...args)
	{
		super(...args);

		this.playerConfig = Engine.GetGUIObjectByName("playerConfig[" + this.playerIndex + "]");

		this.isPageOpen = false;
		this.guid = undefined;
		this.fixedAI = undefined;

		this.defaultAIDiff = +Engine.ConfigDB_GetValue("user", this.ConfigDifficulty);
		this.defaultBehavior = Engine.ConfigDB_GetValue("user", this.ConfigBehavior);

		// Save little performance by not reallocating every call
		this.sprintfArgs = {};

		this.playerConfig.onPress = this.openConfigPage.bind(this, this.playerIndex);
	}

	onMapChange(mapData)
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		let isScenario = g_GameAttributes.mapType == "scenario";

		let mapPData = this.gameSettingsControl.getPlayerData(mapData, this.playerIndex);
		if (mapPData && mapPData.AI)
		{
			let defaultPData = g_Settings.PlayerDefaults[this.playerIndex + 1];

			this.fixedAI = {
				"AI": mapPData.AI,
				"AIDiff":
					mapPData.AIDiff !== undefined ?
						mapPData.AIDiff :
						defaultPData.AIDiff,
				"AIBehavior":
					mapPData.AIBehavior !== undefined ?
						mapPData.AIBehavior :
						defaultPData.AIBehavior
			};
		}
		else
			this.fixedAI = undefined;
	}

	onAssignPlayer(source, target)
	{
		if (source && target.AI)
		{
			source.AI = target.AI;
			source.AIDiff = target.AIDiff;
			source.AIBehavior = target.AIBehavior;
		}

		target.AI = false;
		delete target.AIDiff;
		delete target.AIBehavior;
	}

	onPlayerAssignmentsChange()
	{
		this.guid = undefined;

		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player == this.playerIndex + 1)
				this.guid = guid;
	}

	onGameAttributesChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		// Enforce map specified AI
		if (this.fixedAI &&
			(pData.AI !== this.fixedAI.AI ||
			 pData.AIDiff !== this.fixedAI.AIDiff ||
			 pData.AIBehavior !== this.fixedAI.AIBehavior))
		{
			pData.AI = this.fixedAI.AI;
			pData.AIDiff = this.fixedAI.AIDiff;
			pData.AIBehavior = this.fixedAI.AIBehavior;
			this.gameSettingsControl.updateGameAttributes();
		}

		// Sanitize, make AI state self-consistent
		if (pData.AI)
		{
			if (!pData.AIDiff || !pData.AIBehavior)
			{
				if (!pData.AIDiff)
					pData.AIDiff = this.defaultAIDiff;

				if (!pData.AIBehavior)
					pData.AIBehavior = this.defaultBehavior;

				this.gameSettingsControl.updateGameAttributes();
			}
		}
		else if (pData.AI === undefined)
		{
			if (this.guid)
				pData.AI = false;
			else
			{
				pData.AI = g_Settings.PlayerDefaults[this.playerIndex + 1].AI;
				pData.AIDiff = this.defaultAIDiff;
				pData.AIBehavior = this.defaultBehavior;
			}
			this.gameSettingsControl.updateGameAttributes();
		}
		else if (pData.AIBehavior || pData.AIDiff)
		{
			pData.AI = false;
			delete pData.AIBehavior;
			delete pData.AIDiff;
			this.gameSettingsControl.updateGameAttributes();
		}
	}

	onGameAttributesBatchChange()
	{
		let isPageOpen = this.isPageOpen;
		if (isPageOpen)
			Engine.PopGuiPage();

		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		this.sprintfArgs.description = translateAISettings(pData);
		this.playerConfig.tooltip = sprintf(this.Tooltip, this.sprintfArgs);
		this.playerConfig.hidden = !pData.AI;

		if (isPageOpen)
			this.openConfigPage();
	}

	openConfigPage()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData || !pData.AI)
			return;

	    this.isPageOpen = true;
		Engine.PushGuiPage(
			"page_aiconfig.xml",
			{
				"playerSlot": this.playerIndex,
				"id": pData.AI,
				"difficulty": pData.AIDiff,
				"behavior": pData.AIBehavior,
				"fixed": !!this.fixedAI
			},
			this.onConfigPageClosed.bind(this));
	}

	onConfigPageClosed(data)
	{
		this.isPageOpen = false;

		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!data || !data.save || !g_IsController || !pData)
			return;

		pData.AI = data.id;
		pData.AIDiff = data.difficulty;
		pData.AIBehavior = data.behavior;

		this.gameSettingsControl.updateGameAttributes();
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

PlayerSettingControls.AIConfigButton.prototype.Tooltip =
	translate("Configure AI: %(description)s.");

PlayerSettingControls.AIConfigButton.prototype.ConfigDifficulty =
	"gui.gamesetup.aidifficulty";

PlayerSettingControls.AIConfigButton.prototype.ConfigBehavior =
	"gui.gamesetup.aibehavior";
