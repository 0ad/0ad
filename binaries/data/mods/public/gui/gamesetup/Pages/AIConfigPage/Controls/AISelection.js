AIGameSettingControls.AISelection = class extends AIGameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		g_GameSettings.playerAI.watch(() => this.render(), ["values"]);

		this.values = prepareForDropdown([
			this.NoAI,
			...g_Settings.AIDescriptions.map(AI => ({
				"Title": AI.data.name,
				"Id": AI.id
			}))
		]);

		this.dropdown.list = this.values.Title;
		this.dropdown.list_data = this.values.Id;
	}

	render()
	{
		let ai = g_GameSettings.playerAI.get(this.playerIndex);
		this.setHidden(!ai);
		if (!!ai)
			this.setSelectedValue(ai.bot);
		else
			this.setSelectedValue(undefined);
	}

	onSelectionChange(itemIdx)
	{
		g_GameSettings.playerAI.setAI(this.playerIndex, this.dropdown.list_data[itemIdx]);
		this.gameSettingsControl.setNetworkGameAttributes();
	}
};

AIGameSettingControls.AISelection.prototype.NoAI = {
	"Title": translateWithContext("ai", "None"),
	"Id": undefined
};

AIGameSettingControls.AISelection.prototype.TitleCaption =
	translate("AI Player");
