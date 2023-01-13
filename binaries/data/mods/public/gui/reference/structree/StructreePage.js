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

		this.StructreePage = Engine.GetGUIObjectByName("structreePage");
		this.Background = Engine.GetGUIObjectByName("background");
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

		this.StructreePage.onWindowResized = this.updatePageWidth.bind(this);
		this.width = 0;
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
		this.CivHistory.caption = this.civData[this.activeCiv].History || "";

		let templateLists = this.TemplateLister.getTemplateLists(this.activeCiv);
		this.TreeSection.draw(templateLists.structures, this.activeCiv);
		this.TrainerSection.draw(templateLists.units, this.activeCiv);

		this.width = this.TreeSection.width + -this.TreeSection.rightMargin;
		if (this.TrainerSection.isVisible())
			this.width += this.TrainerSection.width + this.SectionGap;
		this.updatePageWidth();
		this.updatePageHeight();
	}

	updatePageHeight()
	{
		let y = (this.TreeSection.height + this.TreeSection.vMargin) / 2;
		let pageSize = this.StructreePage.size;
		pageSize.top = -y;
		pageSize.bottom = y;
		this.StructreePage.size = pageSize;
	}

	updatePageWidth()
	{
		let screenSize = this.Background.getComputedSize();
		let pageSize = this.StructreePage.size;
		let x = Math.min(this.width, screenSize.right - this.BorderMargin * 2) / 2;
		pageSize.left = -x;
		pageSize.right = x;
		this.StructreePage.size = pageSize;
	}
}

StructreePage.prototype.CloseButtonTooltip =
	translate("%(hotkey)s: Close Structure Tree.");

// Gap between the `TreeSection` and `TrainerSection` gui objects (when the latter is visible)
StructreePage.prototype.SectionGap = 12;

// Margin around the edge of the structree on lower resolutions,
// preventing the UI from being clipped by the edges of the screen.
StructreePage.prototype.BorderMargin = 16;
