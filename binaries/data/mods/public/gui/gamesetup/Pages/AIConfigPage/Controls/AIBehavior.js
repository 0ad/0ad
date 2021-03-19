AIGameSettingControls.AIBehavior = class extends AIGameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.playerAI.watch(() => this.render(), ["values"]);
	}

	render()
	{
		this.dropdown.list = g_Settings.AIBehaviors.map(AIBehavior => AIBehavior.Title);
		this.dropdown.list_data = g_Settings.AIBehaviors.map(AIBehavior => AIBehavior.Name);

		let ai = g_GameSettings.playerAI.get(this.playerIndex);
		this.setHidden(!ai);
		if (!!ai)
			this.setSelectedValue(ai.behavior);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.playerAI.setBehavior(this.playerIndex, this.dropdown.list_data[itemIdx]);
		this.gameSettingsControl.setNetworkInitAttributes();
	}
};

AIGameSettingControls.AIBehavior.prototype.TitleCaption =
	translate("AI Behavior");
