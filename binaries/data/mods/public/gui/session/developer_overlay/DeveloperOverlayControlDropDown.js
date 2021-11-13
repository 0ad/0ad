/**
 * This class sets up a dropdown in the developer overlay and assigns its specific handler.
 */
class DeveloperOverlayControlDropDown extends DeveloperOverlayControl
{
	constructor(handler, i)
	{
		super(handler, i);

		this.dropdown = Engine.GetGUIObjectByName("dev_command_dropdown[" + i + "]");
		this.dropdown.onSelectionChange = this.onSelectionChange.bind(this);
		this.dropdown.hidden = false;
	}

	onSelectionChange()
	{
		this.handler.onSelectionChange(this.dropdown.selected);
		this.update();
	}

	update()
	{
		this.dropdown.list = this.handler.values().map(e => e.label);
		this.dropdown.list_data = this.handler.values().map(e => e.value);
		if (this.handler.selected && this.dropdown.selected != this.handler.selected())
			this.dropdown.selected = this.handler.selected();
		if (this.handler.enabled)
			this.dropdown.enabled = this.handler.enabled();
	}

	setHidden(hidden)
	{
		if (hidden)
			unregisterSimulationUpdateHandler(this.updater);
		else
			registerSimulationUpdateHandler(this.updater);
	}
}
