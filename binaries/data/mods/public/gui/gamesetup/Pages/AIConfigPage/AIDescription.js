class AIDescription
{
	constructor(aiConfigPage, setupWindow)
	{
		this.playerIndex = undefined;

		this.aiDescription = Engine.GetGUIObjectByName("aiDescription");

		aiConfigPage.registerOpenPageHandler(this.onOpenPage.bind(this));

		g_GameSettings.playerAI.watch(() => this.render(), ["values"]);
	}

	onOpenPage(playerIndex)
	{
		this.playerIndex = playerIndex;
		this.render();
	}

	render()
	{
		let AI = g_GameSettings.playerAI.get(this.playerIndex);
		if (!!AI)
			AI = g_Settings.AIDescriptions.find(desc => desc.id == AI.bot);
		this.aiDescription.caption = AI ? AI.data.description : this.NoAIDescription;
	}
}

AIDescription.prototype.NoAIDescription =
	translate("AI will be disabled for this player.");
