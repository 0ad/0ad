/**
 * This class sets up a checkbox in the developer overlay and assigns its specific handler.
 */
class DeveloperOverlayCheckbox
{
	constructor(handler, i)
	{
		this.handler = handler;

		this.label = Engine.GetGUIObjectByName("dev_command_label[" + i + "]");
		this.label.caption = this.handler.label();

		this.checkbox = Engine.GetGUIObjectByName("dev_command_checkbox[" + i + "]");
		this.checkbox.onPress = this.onPress.bind(this);

		this.body = Engine.GetGUIObjectByName("dev_command[" + i + "]");
		this.resize(i);

		this.updater = this.update.bind(this);

		if (this.handler.checked)
			registerPlayersInitHandler(this.updater);
	}

	onPress()
	{
		this.handler.onPress(this.checkbox.checked);

		if (this.handler.checked)
			this.update();
	}

	update()
	{
		this.checkbox.checked = this.handler.checked();
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

	getHeight()
	{
		return this.body.size.bottom - this.body.size.top;
	}

	resize(i)
	{
		let size = this.body.size;
		let height = size.bottom;
		size.top = height * i;
		size.bottom = height * (i + 1);
		this.body.size = size;
		this.body.hidden = false;
	}
}
