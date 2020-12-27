/**
 * This class is implemented by gamesettings that are controlled by a button.
 */
class GameSettingControlButton extends GameSettingControl
{
	setControl(gameSettingControlManager)
	{
		let row = gameSettingControlManager.getNextRow("buttonSettingFrame");
		this.frame = Engine.GetGUIObjectByName("buttonSettingFrame[" + row + "]");
		this.button = Engine.GetGUIObjectByName("buttonSettingControl[" + row + "]");
		this.button.onPress = this.onPress.bind(this);
		if (this.Caption)
			this.button.caption = this.Caption;
	}

	setControlTooltip(tooltip)
	{
		this.button.tooltip = tooltip;
	}

	setControlHidden(hidden)
	{
		this.button.hidden = hidden;
	}
}
