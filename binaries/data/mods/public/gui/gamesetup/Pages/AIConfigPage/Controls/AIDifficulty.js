AIGameSettingControls.AIDifficulty = class extends AIGameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.playerAI.watch(() => this.render(), ["values"]);
	}

	render()
	{
		this.dropdown.list = g_Settings.AIDifficulties.map(AI => AI.Title);
		this.dropdown.list_data = g_Settings.AIDifficulties.map((AI, i) => i);

		let ai = g_GameSettings.playerAI.get(this.playerIndex);
		this.setHidden(!ai);
		if (!!ai)
			this.setSelectedValue(ai.difficulty);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.playerAI.setDifficulty(this.playerIndex, this.dropdown.list_data[itemIdx]);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

AIGameSettingControls.AIDifficulty.prototype.ConfigDifficulty =
	"gui.gamesetup.aidifficulty";

AIGameSettingControls.AIDifficulty.prototype.TitleCaption =
	translate("AI Difficulty");
