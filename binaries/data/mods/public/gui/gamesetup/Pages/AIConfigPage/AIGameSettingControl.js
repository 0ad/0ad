class AIGameSettingControlDropdown extends GameSettingControlDropdown
{
	onOpenPage(playerIndex)
	{
		this.playerIndex = playerIndex;
		this.render();
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
}

AIGameSettingControlDropdown.prototype.Height= 28;

AIGameSettingControlDropdown.prototype.Margin= 7;
