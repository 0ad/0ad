/**
 * This displays the emblem of the civilization of the currently viewed player in the top panel.
 * If clicked, it opens the Structure Tree or Civilization Overview dialog, for the last viewed civilization.
 */
class CivIcon
{
	constructor(playerViewControl)
	{
		this.dialogSelection = {
			"page": "page_structree.xml",
			"args": {
				"civ": undefined
			}
		};

		this.civIcon = Engine.GetGUIObjectByName("civIcon");
		this.civIconOverlay = Engine.GetGUIObjectByName("civIconOverlay");
		this.civIconOverlay.onPress = this.onPress.bind(this);

		playerViewControl.registerViewedPlayerChangeHandler(this.rebuild.bind(this));
		registerHotkeyChangeHandler(this.rebuild.bind(this));

		Engine.SetGlobalHotkey("structree", "Press", this.openPage.bind(this, "page_structree.xml"));
		Engine.SetGlobalHotkey("civinfo", "Press", this.openPage.bind(this, "page_civinfo.xml"));
	}

	onPress()
	{
		this.openPage(this.dialogSelection.page);
	}

	openPage(page)
	{
		closeOpenDialogs();
		g_PauseControl.implicitPause();

		pageLoop(
			page,
			{
				// If an Observer triggers `openPage()` via hotkey, g_ViewedPlayer could be -1 or 0
				// (depending on whether they're "viewing" no-one or gaia respectively)
				"civ": this.dialogSelection.args.civ ?? g_Players[Math.max(g_ViewedPlayer, 1)].civ

				// TODO add info about researched techs and unlocked entities
			},
			data =>
			{
				this.dialogSelection = data;
				resumeGame();
			});
	}

	rebuild()
	{
		let hidden = g_ViewedPlayer <= 0;
		this.civIcon.hidden = hidden;
		if (hidden)
			return;

		let civData = g_CivData[g_Players[g_ViewedPlayer].civ];

		this.civIcon.sprite = "stretched:" + civData.Emblem;
		this.civIconOverlay.tooltip = sprintf(translate(this.Tooltip), {
			"civ": setStringTags(civData.Name, this.CivTags),
			"hotkey_civinfo": colorizeHotkey("%(hotkey)s", "civinfo"),
			"hotkey_structree": colorizeHotkey("%(hotkey)s", "structree")
		});
	}
}

CivIcon.prototype.Tooltip =
	markForTranslation("%(civ)s\n%(hotkey_civinfo)s / %(hotkey_structree)s: View Civilization Overview / Structure Tree\nLast opened will be reopened on click.");

CivIcon.prototype.CivTags = { "font": "sans-bold-stroke-14" };
