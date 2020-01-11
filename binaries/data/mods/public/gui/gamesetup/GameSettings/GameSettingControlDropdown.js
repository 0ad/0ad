/**
 * This class is implemented by gamesettings that are controlled by a dropdown.
 */
class GameSettingControlDropdown extends GameSettingControl
{
	constructor(...args)
	{
		super(...args);

		this.isInGuiUpdate = false;
		this.dropdown.onSelectionChange = this.onSelectionChangeSuper.bind(this);
		if (this.onHoverChange)
			this.dropdown.onHoverChange = this.onHoverChange.bind(this);
	}

	setControl(gameSettingControlManager)
	{
		let row = gameSettingControlManager.getNextRow("dropdownSettingFrame");
		this.frame = Engine.GetGUIObjectByName("dropdownSettingFrame[" + row + "]");
		this.dropdown = Engine.GetGUIObjectByName("dropdownSettingControl[" + row + "]");

		let labels = this.frame.children[0].children;
		this.title = labels[0];
		this.label = labels[1];
	}

	setControlTooltip(tooltip)
	{
		this.dropdown.tooltip = tooltip;
	}

	setControlHidden(hidden)
	{
		this.dropdown.hidden = hidden;
	}

	setSelectedValue(value)
	{
		let index = this.dropdown.list_data.indexOf(String(value));

		this.isInGuiUpdate = true;
		this.dropdown.selected = index;
		this.isInGuiUpdate = false;

		if (this.label)
			this.label.caption = index == -1 ? this.UnknownValue : this.dropdown.list[index];
	}

	onSelectionChangeSuper()
	{
		if (!this.isInGuiUpdate)
			this.onSelectionChange(this.dropdown.selected);
	}
}

/**
 * Highlight the "random" dropdownlist item.
 */
GameSettingControlDropdown.prototype.RandomItemTags = {
	"color": "orange"
};

GameSettingControlDropdown.prototype.UnknownValue =
	translateWithContext("settings value", "Unknown");
