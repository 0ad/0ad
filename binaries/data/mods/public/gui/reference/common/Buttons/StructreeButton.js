class StructreeButton
{
	constructor(parentPage)
	{
		this.parentPage = parentPage;

		this.civInfoButton = Engine.GetGUIObjectByName("structreeButton");
		this.civInfoButton.onPress = this.onPress.bind(this);
		this.civInfoButton.caption = this.Caption;
		this.civInfoButton.tooltip = colorizeHotkey(this.Tooltip, this.Hotkey);
	}

	onPress()
	{
		Engine.PopGuiPage({ "civ": this.parentPage.activeCiv, "nextPage": "page_structree.xml" });
	}

}

StructreeButton.prototype.Caption =
	translate("Structure Tree");

StructreeButton.prototype.Hotkey =
	"structree";

StructreeButton.prototype.Tooltip =
	translate("%(hotkey)s: Switch to Structure Tree.");
