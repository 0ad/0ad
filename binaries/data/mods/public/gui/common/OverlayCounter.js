/**
 * This is an abstract base class managing one counter shown.
 * Classes implementing this class require a Config property and may have a Hotkey property.
 */
class OverlayCounter
{
	constructor(overlayCounterManager)
	{
		this.overlayCounterManager = overlayCounterManager;
		this.updateEnabled();

		registerConfigChangeHandler(this.onConfigChange.bind(this));

		if (this.Hotkey)
			Engine.SetGlobalHotkey(this.Hotkey, "Press", this.toggle.bind(this));
	}

	onConfigChange(changes)
	{
		if (changes.has(this.Config))
			this.updateEnabled();
	}

	isEnabled()
	{
		return Engine.ConfigDB_GetValue("user", this.Config) == "true";
	}

	updateEnabled()
	{
		this.overlayCounterManager.setCounterEnabled(this, this.isEnabled());
	}

	toggle()
	{
		Engine.ConfigDB_CreateValue("user", this.Config, String(!this.isEnabled()));
		this.updateEnabled();
	}
}

/**
 * The properties of this prototype are defined in other files. Each of them is a class
 * managing a counter shown on the current page and may extend the OverlayCounter class.
 */
class OverlayCounterTypes
{
}
