/**
 * This class updates the diplomacy colors button within the diplomacy dialog.
 */
class DiplomacyDialogColorsButton
{
	constructor(diplomacyColors)
	{
		this.diplomacyColorsWindowButton = Engine.GetGUIObjectByName("diplomacyColorsWindowButton");
		this.diplomacyColorsWindowButtonIcon = Engine.GetGUIObjectByName("diplomacyColorsWindowButtonIcon");
		this.diplomacyColorsWindowButton.onPress = diplomacyColors.toggle.bind(diplomacyColors);
		registerHotkeyChangeHandler(this.onHotkeyChange.bind(this));
		diplomacyColors.registerDiplomacyColorsChangeHandler(this.onDiplomacyColorsChange.bind(this))
	}

	onHotkeyChange()
	{
		this.diplomacyColorsWindowButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "session.diplomacycolors") +
			translate(this.Tooltip);
	}

	onDiplomacyColorsChange(enabled)
	{
		this.diplomacyColorsWindowButtonIcon.sprite =
			"stretched:" +
			(enabled ? this.SpriteEnabled : this.SpriteDisabled);
	}
}

DiplomacyDialogColorsButton.prototype.Tooltip = markForTranslation("Toggle Diplomacy Colors");

DiplomacyDialogColorsButton.prototype.SpriteEnabled = "session/icons/diplomacy-on.png";

DiplomacyDialogColorsButton.prototype.SpriteDisabled = "session/icons/diplomacy.png";
