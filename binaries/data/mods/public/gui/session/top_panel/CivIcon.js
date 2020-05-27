/**
 * This displays the emblem of the civilization of the currently viewed player in the top panel.
 * If clicked, it opens the Structure Tree or Civilization Overview dialog, for the last viewed civilization.
 */
class CivIcon
{
	constructor(playerViewControl)
	{
		this.dialogSelection = {
			"civ": "",
			"page": "page_structree.xml"
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

		Engine.PushGuiPage(
			page,
			{
				"civ": this.dialogSelection.civ || g_Players[g_ViewedPlayer].civ
				// TODO add info about researched techs and unlocked entities
			},
			this.storePageSelection.bind(this));
	}

	storePageSelection(data)
	{
		if (data.nextPage)
			Engine.PushGuiPage(
				data.nextPage,
				{ "civ": data.civ },
				this.storePageSelection.bind(this));
		else
		{
			this.dialogSelection = data;
			resumeGame();
		}
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
