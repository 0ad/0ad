/**
 * This code wraps the gui representing buildable structures within the structree.
 *
 * An instance of this class is created for each child of the gui element named "structures".
 */
class StructureBox extends EntityBox
{
	constructor(page, structureIdx)
	{
		super(page);
		this.gui = Engine.GetGUIObjectByName("structure[" + structureIdx + "]");
		this.ProductionRows = new ProductionRowManager(this.page, "structure[" + structureIdx + "]_productionRows", true);
	}

	draw(templateName, civCode, runningWidths)
	{
		super.draw(templateName, civCode);

		this.phaseIdx = this.page.TemplateParser.phaseList.indexOf(this.template.phase);

		// Draw the production rows
		this.ProductionRows.draw(this.template, civCode, this.phaseIdx);

		let boxWidth = Math.max(this.MinWidth, this.captionWidth(), this.ProductionRows.width);

		// Set position of the Structure Box
		let size = this.gui.size;
		size.left = this.HMargin + runningWidths[this.phaseIdx];
		size.right = this.HMargin + runningWidths[this.phaseIdx] + boxWidth;
		size.top = TreeSection.getPositionOffset(this.phaseIdx, this.page.TemplateParser);
		size.bottom = TreeSection.getPositionOffset(this.phaseIdx + 1, this.page.TemplateParser) - this.VMargin;
		this.gui.size = size;

		// Update new right-side-edge dimension
		runningWidths[this.phaseIdx] += boxWidth + this.HMargin / 2;
	}
}
