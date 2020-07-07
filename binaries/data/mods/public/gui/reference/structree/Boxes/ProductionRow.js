class ProductionRow
{
	constructor(page, guiObject, rowIndex)
	{
		this.page = page;
		this.productionRow = guiObject;
		this.productionIconsDrawn = 0;
		this.rowIndex = rowIndex;
		this.phaseOffset = 0;

		horizontallySpaceObjects(this.productionRow.name, ProductionIcon.Size().hMargin);

		this.productionIcons = [];
		for (let icon of guiObject.children)
			this.productionIcons.push(new ProductionIcon(this.page, icon));
	}

	startDraw(phaseOffset)
	{
		this.productionIconsDrawn = 0;
		this.phaseOffset = phaseOffset;
	}

	drawIcon(template, civCode)
	{
		if (this.productionIconsDrawn == this.productionIcons.length)
		{
			error("The currently displayed civ has more production options " +
					  "than can be supported by the current GUI layout");
			return;
		}

		this.productionIcons[this.productionIconsDrawn].draw(template, civCode);
		++this.productionIconsDrawn;
	}

	finishDraw()
	{
		hideRemaining(this.productionRow.name, this.productionIconsDrawn);

		const IconSize = ProductionIcon.Size();
		let rowOffset = IconSize.rowHeight * (this.phaseOffset - this.rowIndex);
		let rowWidth = this.productionIconsDrawn * IconSize.rowWidth + IconSize.hMargin;

		let size = this.productionRow.size;
		size.left = -rowWidth / 2;
		size.top = -rowOffset;
		this.productionRow.size = size;

		this.productionRow.hidden = false;
		return rowWidth;
	}

	hide()
	{
		this.productionRow.hidden = true
	}
}
