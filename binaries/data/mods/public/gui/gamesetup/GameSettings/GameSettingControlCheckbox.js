/**
 * This class is implemented by gamesettings that are controlled by a checkbox.
 */
class GameSettingControlCheckbox extends GameSettingControl
{
	constructor(...args)
	{
		super(...args);

		this.isInGuiUpdate = false;
		this.previousSelectedValue = undefined;
	}

	setControl(gameSettingControlManager)
	{
		let row = gameSettingControlManager.getNextRow("checkboxSettingFrame");
		this.frame = Engine.GetGUIObjectByName("checkboxSettingFrame[" + row + "]");
		this.checkbox = Engine.GetGUIObjectByName("checkboxSettingControl[" + row + "]");
		this.checkbox.onPress = this.onPressSuper.bind(this);

		let labels = this.frame.children[0].children;
		this.title = labels[0];
		this.label = labels[1];
	}

	setControlTooltip(tooltip)
	{
		this.checkbox.tooltip = tooltip;
	}

	setControlHidden(hidden)
	{
		this.checkbox.hidden = hidden;
	}

	setChecked(checked)
	{
		if (this.previousSelectedValue == checked)
			return;

		this.isInGuiUpdate = true;
		this.checkbox.checked = checked;
		this.isInGuiUpdate = false;

		if (this.label)
			this.label.caption = checked ? this.Checked : this.Unchecked;
	}

	onPressSuper()
	{
		if (!this.isInGuiUpdate)
			this.onPress(this.checkbox.checked);
	}
}

GameSettingControlCheckbox.prototype.Checked =
	translate("Yes");

GameSettingControlCheckbox.prototype.Unchecked =
	translate("No");
