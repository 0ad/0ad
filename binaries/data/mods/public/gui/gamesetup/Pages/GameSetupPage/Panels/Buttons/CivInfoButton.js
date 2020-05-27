class CivInfoButton
{
	constructor()
	{
		this.civInfo = {
			"civ": "",
			"page": "page_civinfo.xml"
		};

		let civInfoButton = Engine.GetGUIObjectByName("civInfoButton");
		civInfoButton.onPress = this.onPress.bind(this);
		civInfoButton.tooltip =
			sprintf(translate(this.Tooltip), {
				"hotkey_civinfo": colorizeHotkey("%(hotkey)s", "civinfo"),
				"hotkey_structree": colorizeHotkey("%(hotkey)s", "structree")
			});

		Engine.SetGlobalHotkey("structree", "Press", this.openPage.bind(this, "page_structree.xml"));
		Engine.SetGlobalHotkey("civinfo", "Press", this.openPage.bind(this, "page_civinfo.xml"));
	}

	onPress()
	{
		this.openPage(this.civInfo.page);
	}

	openPage(page)
	{
		Engine.PushGuiPage(
			page,
			{ "civ": this.civInfo.civ },
			this.storeCivInfoPage.bind(this));
	}

	storeCivInfoPage(data)
	{
		if (data.nextPage)
			Engine.PushGuiPage(
				data.nextPage,
				{ "civ": data.civ },
				this.storeCivInfoPage.bind(this));
		else
			this.civInfo = data;
	}
}

CivInfoButton.prototype.Tooltip =
	translate("%(hotkey_civinfo)s / %(hotkey_structree)s: View Civilization Overview / Structure Tree\nLast opened will be reopened on click.");
