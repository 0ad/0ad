/**
 * This class handles the button which opens the diplomacy dialog.
 */
class DiplomacyDialogButton
{
	constructor(playerViewControl, diplomacyDialog)
	{
		this.diplomacyButton = Engine.GetGUIObjectByName("diplomacyButton");
		this.diplomacyButton.enabled = !Engine.IsAtlasRunning();
		this.diplomacyButton.onPress = diplomacyDialog.toggle.bind(diplomacyDialog);

		registerHotkeyChangeHandler(this.onHotkeyChange.bind(this));
		playerViewControl.registerViewedPlayerChangeHandler(this.onViewedPlayerChange.bind(this));
	}

	onHotkeyChange()
	{
		this.diplomacyButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "session.gui.diplomacy.toggle") +
			translate(this.Tooltip);
	}

	onViewedPlayerChange()
	{
		this.diplomacyButton.hidden = g_ViewedPlayer < 1;
	}
}

DiplomacyDialogButton.prototype.Tooltip = markForTranslation("Diplomacy");
