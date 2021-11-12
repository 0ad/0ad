/**
 * This class sets up a checkbox in the developer overlay and assigns its specific handler.
 */
class DeveloperOverlayControlCheckbox extends DeveloperOverlayControl
{
	constructor(handler, i)
	{
		super(handler, i);

		this.label = Engine.GetGUIObjectByName("dev_command_label[" + i + "]");
		this.label.caption = this.handler.label();
		this.label.hidden = false;

		this.checkbox = Engine.GetGUIObjectByName("dev_command_checkbox[" + i + "]");
		this.checkbox.onPress = this.onPress.bind(this);
		this.checkbox.hidden = false;
	}

	onPress()
	{
		this.handler.onPress(this.checkbox.checked);
		this.update();
	}

	update()
	{
		if (this.handler.checked)
			this.checkbox.checked = this.handler.checked();
		if (this.handler.enabled)
			this.checkbox.enabled = this.handler.enabled();
	}

	setHidden(hidden)
	{
		if (!this.handler.checked)
			return;

		if (hidden)
			unregisterSimulationUpdateHandler(this.updater);
		else
			registerSimulationUpdateHandler(this.updater);
	}
}

