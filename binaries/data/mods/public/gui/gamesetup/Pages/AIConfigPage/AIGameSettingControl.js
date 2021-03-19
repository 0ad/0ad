class AIGameSettingControlDropdown extends GameSettingControlDropdown
{
	onOpenPage(playerIndex)
	{
		this.setEnabled(true);
		this.playerIndex = playerIndex;
		this.render();
	}

	/**
	 * Overloaded: no need to trigger a relayout,
	 * but updateVisibility must be called manually
	 * as the AI control manager does not subscribe to updateLayout.
	 */
	setHidden(hidden)
	{
		this.hidden = hidden;
		this.updateVisibility();
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
