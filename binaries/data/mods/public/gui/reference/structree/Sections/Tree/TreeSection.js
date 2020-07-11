class TreeSection
{
	constructor(page)
	{
		this.page = page;

		this.TreeSection = Engine.GetGUIObjectByName("treeSection");
		this.Structures = Engine.GetGUIObjectByName("structures");

		this.PhaseIdents = new PhaseIdentManager(this.page);

		this.rightMargin = this.TreeSection.size.right;
		this.vMargin = this.TreeSection.size.top + -this.TreeSection.size.bottom;
		this.width = 0;
		this.height = 0;

		this.structureBoxes = [];
		for (let boxIdx in this.Structures.children)
			this.structureBoxes.push(new StructureBox(this.page, boxIdx));

		page.TrainerSection.registerWidthChangedHandler(this.onTrainerSectionWidthChange.bind(this));
	}

	draw(structures, civCode)
	{
		if (structures.size > this.structureBoxes.length)
			error("\"" + this.activeCiv + "\" has more structures than can be supported by the current GUI layout");

		// Draw structures
		let phaseList = this.page.TemplateParser.phaseList;
		let count = Math.min(structures.size, this.structureBoxes.length);
		let runningWidths = Array(phaseList.length).fill(0);
		let structureIterator = structures.keys();
		for (let idx = 0; idx < count; ++idx)
			this.structureBoxes[idx].draw(structureIterator.next().value, civCode, runningWidths);
		hideRemaining(this.Structures.name, count);

		// Position phase idents
		this.PhaseIdents.draw(phaseList, civCode, runningWidths, this.Structures.size.left);

		this.width = this.Structures.size.left + Math.max(...runningWidths) + EntityBox.prototype.HMargin;
		this.height = this.constructor.getPositionOffset(phaseList.length, this.page.TemplateParser);
	}

	drawPhaseIcon(phaseIcon, phaseIndex, civCode)
	{
		let phaseName = this.page.TemplateParser.phaseList[phaseIndex];
		let prodPhaseTemplate = this.page.TemplateParser.getTechnology(phaseName + "_" + civCode, civCode) || this.page.TemplateParser.getTechnology(phaseName, civCode);

		phaseIcon.sprite = "stretched:" + this.page.IconPath + prodPhaseTemplate.icon;
		phaseIcon.tooltip = getEntityNamesFormatted(prodPhaseTemplate);
	};

	onTrainerSectionWidthChange(trainerSectionWidth, trainerSectionVisible)
	{
		let size = this.TreeSection.size;
		size.right = this.rightMargin;
		if (trainerSectionVisible)
			size.right -= trainerSectionWidth + this.page.SectionGap;
		this.TreeSection.size = size;
	}

	/**
	 * Calculate row position offset (accounting for different number of prod rows per phase).
	 *
	 * This is a static method as it is also used from within the StructureBox and PhaseIdent classes.
	 *
	 * @param {number} idx
	 * @return {number}
	 */
	static getPositionOffset(idx, TemplateParser)
	{
		let phases = TemplateParser.phaseList.length;
		let rowHeight = ProductionIcon.Size().rowHeight;

		let size = EntityBox.IconAndCaptionHeight() * idx; // text, image and offset
		size += EntityBox.prototype.VMargin * (idx + 1); // Margin above StructureBoxes
		size += rowHeight * (phases * idx - (idx - 1) * idx / 2); // phase rows (phase-currphase+1 per row)

		return size;
	};

}
