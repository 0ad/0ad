/**
 * If the button that this class manages is pressed, an idle unit having one of the given classes is selected.
 */
class MiniMapIdleWorkerButton
{
	constructor(idleClasses)
	{
		this.idleWorkerButton = Engine.GetGUIObjectByName("idleWorkerButton");
		this.idleWorkerButton.onPress = this.onPress.bind(this);
		this.idleClasses = idleClasses;
	}

	update()
	{
		this.idleWorkerButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "selection.idleworker") +
			translate(this.Tooltip);

		this.idleWorkerButton.enabled = Engine.GuiInterfaceCall("HasIdleUnits", {
			"viewedPlayer": g_ViewedPlayer,
			"idleClasses": this.idleClasses,
			"excludeUnits": []
		});
	}

	onPress()
	{
		findIdleUnit(this.idleClasses);
	}
}

MiniMapIdleWorkerButton.prototype.Tooltip = markForTranslation("Find idle worker");
