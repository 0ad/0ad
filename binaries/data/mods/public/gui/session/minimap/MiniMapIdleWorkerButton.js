/**
 * If the button that this class manages is pressed, an idle unit having one of the given classes is selected.
 */
class MiniMapIdleWorkerButton
{
	constructor(playerViewControl, idleClasses)
	{
		this.idleWorkerButton = Engine.GetGUIObjectByName("idleWorkerButton");
		this.idleWorkerButton.onKeyDown = this.onKeyDown.bind(this);
		this.idleWorkerButton.onMouseLeftPress = this.onKeyDown.bind(this);
		this.idleClasses = idleClasses;

		registerHotkeyChangeHandler(this.onHotkeyChange.bind(this));
		registerSimulationUpdateHandler(this.rebuild.bind(this));
		playerViewControl.registerViewedPlayerChangeHandler(this.rebuild.bind(this));
	}

	onHotkeyChange()
	{
		this.idleWorkerButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "selection.idleworker") +
			translate(this.Tooltip);
	}

	rebuild()
	{
		this.idleWorkerButton.enabled = Engine.GuiInterfaceCall("HasIdleUnits", {
			"viewedPlayer": g_ViewedPlayer,
			"idleClasses": this.idleClasses,
			"excludeUnits": []
		});
	}

	onKeyDown()
	{
		findIdleUnit(this.idleClasses);
	}
}

MiniMapIdleWorkerButton.prototype.Tooltip = markForTranslation("Find idle worker");
