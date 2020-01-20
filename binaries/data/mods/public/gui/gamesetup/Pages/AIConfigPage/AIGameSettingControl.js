class AIGameSettingControlDropdown extends GameSettingControlDropdown
{
	constructor(...args)
	{
		super(...args);

		this.gameSettingsControl.registerAssignPlayerHandler(this.onAssignPlayer.bind(this));
	}

	setControl(aiConfigPage)
	{
		aiConfigPage.registerOpenPageHandler(this.onOpenPage.bind(this));

		let i = aiConfigPage.getRow();

		this.frame = Engine.GetGUIObjectByName("aiSettingFrame[" + i + "]");
		this.title = this.frame.children[0];
		this.dropdown = this.frame.children[1];
		this.label = this.frame.children[2];

		let size = this.frame.size;
		size.top = i * (this.Height + this.Margin);
		size.bottom = size.top + this.Height;
		this.frame.size = size;

		this.setHidden(false);
	}

	onOpenPage(playerIndex)
	{
		this.playerIndex = playerIndex;
		this.updateSelectedValue();
		this.updateVisibility();
	}

	onGameAttributesChange()
	{
		for (let playerIndex = 0; playerIndex < g_MaxPlayers; ++playerIndex)
			this.onGameAttributesChangePlayer(playerIndex);
	}

	onGameAttributesBatchChange()
	{
		this.updateSelectedValue();
	}
}

AIGameSettingControlDropdown.prototype.Height= 28;

AIGameSettingControlDropdown.prototype.Margin= 7;
