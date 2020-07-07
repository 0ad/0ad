/**
 * This code wraps the gui representing "trainer units" (a unit that can train other units) within the structree.
 *
 * An instance of this class is created for each child of the gui element named "trainers".
 */
class TrainerBox extends EntityBox
{
	constructor(page, trainerIdx)
	{
		super(page);

		this.gui = Engine.GetGUIObjectByName("trainer[" + trainerIdx + "]");
		this.ProductionRows = new ProductionRowManager(this.page, "trainer[" + trainerIdx + "]_productionRows", false);

		let rowHeight = ProductionIcon.Size().rowHeight;
		let size = this.gui.size;

		// Adjust height to accommodate production row
		size.bottom += rowHeight;

		// We make the assumuption that all trainer boxes have the same height
		let boxHeight = this.VMargin / 2 + (size.bottom - size.top + this.VMargin) * trainerIdx;
		size.top += boxHeight;
		size.bottom += boxHeight;

		// Make the box adjust automatically to column width
		size.rright = 100;
		size.right = -size.left;

		this.gui.size = size;
	}

	draw(templateName, civCode)
	{
		super.draw(templateName, civCode);

		this.ProductionRows.draw(this.template, civCode);

		// Return the box width
		return Math.max(this.MinWidth, this.captionWidth(), this.ProductionRows.width);
	}
}
