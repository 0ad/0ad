/**
 * The purpose of this class is to exclusively manage the diplomacy colors button within the minimap.
 */
class MiniMapDiplomacyColorsButton
{
	constructor(diplomacyColors)
	{
		this.diplomacyColorsButton = Engine.GetGUIObjectByName("diplomacyColorsButton");
		this.diplomacyColorsButton.onPress = diplomacyColors.toggle.bind(diplomacyColors);

		this.diplomacyColors = diplomacyColors;
	}

	update()
	{
		this.diplomacyColorsButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "session.diplomacycolors") +
			translate(this.Tooltip);

		this.diplomacyColorsButton.sprite =
			"stretched:" +
			(this.diplomacyColors.isEnabled() ? this.SpriteEnabled : this.SpriteDisabled);

		this.diplomacyColorsButton.sprite_over =
			"stretched:" +
			(this.diplomacyColors.isEnabled() ? this.SpriteEnabledOver : this.SpriteDisabledOver);
	}
}

MiniMapDiplomacyColorsButton.prototype.Tooltip = markForTranslation("Toggle Diplomacy Colors");

MiniMapDiplomacyColorsButton.prototype.SpriteEnabled = "session/minimap-diplomacy-on.png";
MiniMapDiplomacyColorsButton.prototype.SpriteDisabled = "session/minimap-diplomacy-off.png";

MiniMapDiplomacyColorsButton.prototype.SpriteEnabledOver = "session/minimap-diplomacy-on-highlight.png";
MiniMapDiplomacyColorsButton.prototype.SpriteDisabledOver = "session/minimap-diplomacy-off-highlight.png";
