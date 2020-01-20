PlayerSettingControls.AIConfigButton = class extends GameSettingControl
{
	constructor(...args)
	{
		super(...args);

		this.aiConfigButton = Engine.GetGUIObjectByName("aiConfigButton[" + this.playerIndex + "]");

		// Save little performance by not reallocating every call
		this.sprintfArgs = {};
	}

	onLoad()
	{
		let aiConfigPage = this.setupWindow.pages.AIConfigPage;
		this.aiConfigButton.onPress = aiConfigPage.openPage.bind(aiConfigPage, this.playerIndex);
	}

	onGameAttributesBatchChange()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;

		this.sprintfArgs.description = translateAISettings(pData);
		this.aiConfigButton.tooltip = sprintf(this.Tooltip, this.sprintfArgs);
		this.aiConfigButton.hidden = !pData.AI;
	}
};

PlayerSettingControls.AIConfigButton.prototype.Tooltip =
	translate("Configure AI: %(description)s.");
