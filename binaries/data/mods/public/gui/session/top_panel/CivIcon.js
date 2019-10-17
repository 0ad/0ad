/**
 * This displas the emblem of the civilization of the currently viewed player in the top panel.
 * If clicked, it opens the structure tree or history dialog for the last viewed civilization.
 */
class CivIcon
{
	constructor(playerViewControl)
	{
		this.civIcon = Engine.GetGUIObjectByName("civIcon");
		this.civIconOverlay = Engine.GetGUIObjectByName("civIconOverlay");
		this.civIconOverlay.onPress = this.onPress.bind(this);
		playerViewControl.registerViewedPlayerChangeHandler(this.rebuild.bind(this));
		registerHotkeyChangeHandler(this.rebuild.bind(this));
	}

	onPress()
	{
		openStrucTree(g_CivInfo.page);
	}

	rebuild()
	{
		let hidden = g_ViewedPlayer <= 0;
		this.civIcon.hidden = hidden;
		if (hidden)
			return;

		let civData = g_CivData[g_Players[g_ViewedPlayer].civ];

		this.civIcon.sprite = "stretched:" + civData.Emblem;
		this.civIconOverlay.tooltip = sprintf(translate(this.OverlayTooltip), {
			"civ": setStringTags(civData.Name, this.CivTags),
			"hotkey_civinfo": colorizeHotkey("%(hotkey)s", "civinfo"),
			"hotkey_structree": colorizeHotkey("%(hotkey)s", "structree")
		});
	}
}

CivIcon.prototype.OverlayTooltip =
	markForTranslation("%(civ)s\n%(hotkey_civinfo)s / %(hotkey_structree)s: View History / Structure Tree\nLast opened will be reopened on click.");

CivIcon.prototype.CivTags = { "font": "sans-bold-stroke-14" };
