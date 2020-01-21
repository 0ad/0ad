class AIDescription
{
	constructor(aiConfigPage, setupWindow)
	{
		this.playerIndex = undefined;

		this.aiDescription = Engine.GetGUIObjectByName("aiDescription");

		this.gameSettingsControl = setupWindow.controls.gameSettingsControl;
		this.gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));

		aiConfigPage.registerOpenPageHandler(this.onOpenPage.bind(this));
	}

	onOpenPage(playerIndex)
	{
		this.playerIndex = playerIndex;
		this.updateSelectedValue();
	}

	onGameAttributesBatchChange()
	{
		this.updateSelectedValue();
	}

	updateSelectedValue()
	{
		let pData = this.gameSettingsControl.getPlayerData(g_GameAttributes, this.playerIndex);
		if (!pData)
			return;
		let AI = g_Settings.AIDescriptions.find(AI => AI.id == pData.AI);
		this.aiDescription.caption = AI ? AI.data.description : this.NoAIDescription;
	}
}

AIDescription.prototype.NoAIDescription =
	translate("AI will be disabled for this player.");
