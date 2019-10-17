/**
 * This class handles the button that displays the games objectives.
 */
class ObjectivesDialogButton
{
	constructor(objectivesDialog)
	{
		this.objectivesButton = Engine.GetGUIObjectByName("objectivesButton");
		this.objectivesButton.enabled = !Engine.IsAtlasRunning();
		this.objectivesButton.onPress = objectivesDialog.toggle.bind(objectivesDialog);

		registerHotkeyChangeHandler(this.onHotkeyChange.bind(this));
	}

	onHotkeyChange()
	{
		this.objectivesButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "session.gui.objectives.toggle") +
			translate(this.Tooltip);
	}
}

ObjectivesDialogButton.prototype.Tooltip = markForTranslation("Objectives");
