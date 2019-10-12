/**
 * This class handles the button which opens the diplomacy dialog.
 */
class DiplomacyButton
{
	constructor(diplomacyDialog)
	{
		this.diplomacyButton = Engine.GetGUIObjectByName("diplomacyButton");
		this.diplomacyButton.enabled = !Engine.IsAtlasRunning();
		this.diplomacyButton.onPress = diplomacyDialog.toggle.bind(diplomacyDialog);
	}

	update()
	{
		this.diplomacyButton.hidden = g_ViewedPlayer < 1;
		this.diplomacyButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "session.gui.diplomacy.toggle") +
			translate(this.Tooltip);
	}
}

DiplomacyButton.prototype.Tooltip = markForTranslation("Diplomacy");
