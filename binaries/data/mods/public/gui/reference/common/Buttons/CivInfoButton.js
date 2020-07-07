class CivInfoButton
{
	constructor(parentPage)
	{
		this.parentPage = parentPage;

		this.civInfoButton = Engine.GetGUIObjectByName("civInfoButton");
		this.civInfoButton.onPress = this.onPress.bind(this);
		this.civInfoButton.caption = this.Caption;
		this.civInfoButton.tooltip = colorizeHotkey(this.Tooltip, this.Hotkey);
	}

	onPress()
	{
		Engine.PopGuiPage({ "civ": this.parentPage.activeCiv, "nextPage": "page_civinfo.xml" });
	}

}

CivInfoButton.prototype.Caption =
	translate("Civilization Overview");

CivInfoButton.prototype.Hotkey =
	"civinfo";

CivInfoButton.prototype.Tooltip =
	translate("%(hotkey)s: Switch to Civilization Overview.");
