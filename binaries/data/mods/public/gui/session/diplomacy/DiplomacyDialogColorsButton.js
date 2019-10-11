/**
 * This class updates the diplomacy colors button within the diplomacy dialog.
 */
class DiplomacyDialogColorsButton
{
	constructor(diplomacyColors)
	{
		this.diplomacyColors = diplomacyColors;

		this.diplomacyColorsWindowButton = Engine.GetGUIObjectByName("diplomacyColorsWindowButton");
		this.diplomacyColorsWindowButtonIcon = Engine.GetGUIObjectByName("diplomacyColorsWindowButtonIcon");
		this.diplomacyColorsWindowButton.onPress = diplomacyColors.toggle.bind(diplomacyColors);
	}

	update()
	{
		this.diplomacyColorsWindowButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "session.diplomacycolors") +
			translate(this.Tooltip);

		this.diplomacyColorsWindowButtonIcon.sprite =
			"stretched:" +
			(this.diplomacyColors.isEnabled() ? this.SpriteEnabled : this.SpriteDisabled);
	}
}

DiplomacyDialogColorsButton.prototype.Tooltip = markForTranslation("Toggle Diplomacy Colors");

DiplomacyDialogColorsButton.prototype.SpriteEnabled = "session/icons/diplomacy-on.png";

DiplomacyDialogColorsButton.prototype.SpriteDisabled = "session/icons/diplomacy.png";
