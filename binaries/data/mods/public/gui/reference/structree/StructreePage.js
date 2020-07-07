/**
 * This class represents the Structure Tree GUI page.
 *
 * Further methods are described within draw.js
 */
class StructreePage extends ReferencePage
{
	constructor(data)
	{
		super();

		this.structureBoxes = [];
		this.trainerBoxes = [];

		this.CivEmblem = Engine.GetGUIObjectByName("civEmblem");
		this.CivName = Engine.GetGUIObjectByName("civName");
		this.CivHistory = Engine.GetGUIObjectByName("civHistory");

		this.TrainerSection = new TrainerSection(this);
		this.TreeSection = new TreeSection(this);

		this.civSelection = new CivSelectDropdown(this.civData);
		if (!this.civSelection.hasCivs())
		{
			this.closePage();
			return;
		}
		this.civSelection.registerHandler(this.selectCiv.bind(this));

		let civInfoButton = new CivInfoButton(this);
		let closeButton = new CloseButton(this);
		Engine.SetGlobalHotkey("structree", "Press", this.closePage.bind(this));
	}

	closePage()
	{
		Engine.PopGuiPage({ "civ": this.activeCiv, "page": "page_structree.xml" });
	}

	selectCiv(civCode)
	{
		this.setActiveCiv(civCode);

		this.CivEmblem.sprite = "stretched:" + this.civData[this.activeCiv].Emblem;
		this.CivName.caption = this.civData[this.activeCiv].Name;
		this.CivHistory.caption = this.civData[this.activeCiv].History;

		let templateLists = this.TemplateLister.getTemplateLists(this.activeCiv);
		this.TreeSection.draw(templateLists.structures, this.activeCiv);
		this.TrainerSection.draw(templateLists.units, this.activeCiv);
	}
}

StructreePage.prototype.CloseButtonTooltip =
	translate("%(hotkey)s: Close Structure Tree.");

// Gap between the `TreeSection` and `TrainerSection` gui objects (when the latter is visible)
StructreePage.prototype.SectionGap = 12;
