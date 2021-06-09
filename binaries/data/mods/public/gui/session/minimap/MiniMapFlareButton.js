/**
 * If the button that this class manages is pressed, an idle unit having one of the given classes is selected.
 */
class MiniMapFlareButton
{
	constructor(playerViewControl)
	{
		this.flareButton = Engine.GetGUIObjectByName("flareButton");
		this.flareButton.onPress = this.onPress.bind(this);
		registerHotkeyChangeHandler(this.onHotkeyChange.bind(this));
		playerViewControl.registerViewedPlayerChangeHandler(this.rebuild.bind(this));
	}

	rebuild()
	{
		this.flareButton.enabled = !g_IsObserver;
	}

	onHotkeyChange()
	{
		this.flareButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "session.flare") +
			translate(this.Tooltip);
	}

	onPress()
	{
		if (g_IsObserver)
			return;
		if (inputState == INPUT_NORMAL)
			inputState = INPUT_FLARE;
	}
}

MiniMapFlareButton.prototype.Tooltip = markForTranslation("Send a flare to your allies");
