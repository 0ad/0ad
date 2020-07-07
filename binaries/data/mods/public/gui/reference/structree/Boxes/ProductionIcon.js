class ProductionIcon
{
	constructor(page, guiObject)
	{
		this.page = page;
		this.productionIcon = guiObject;
	}

	/* Returns the dimensions of a single icon, including some "helper" attributes.
	 *
	 * Two assumptions are made: (1) that all production icons are the same size,
	 * and (2) that the size will never change for the duration of the life of the
	 * containing page.
	 *
	 * As such, the method replaces itself after being run once, so the calculations
	 * within are only performed once.
	 */
	static Size()
	{
		let baseObject = Engine.GetGUIObjectByName("phase[0]_bar[0]_icon").size;
		let size = {};

		// Icon dimensions
		size.width = baseObject.right - baseObject.left;
		size.height = baseObject.bottom - baseObject.top;

		// Horizontal and Vertical Margins.
		size.hMargin = baseObject.left;
		size.vMargin = baseObject.top;

		// Width and Height padded with margins on all sides.
		size.paddedWidth = size.width + size.hMargin * 2;
		size.paddedHeight = size.height + size.vMargin * 2;

		// Padded dimensions to use when in production rows.
		size.rowWidth = size.width + size.hMargin;
		size.rowHeight = size.paddedHeight + size.vMargin * 2;
		size.rowGap = size.rowHeight - size.paddedHeight;

		// Replace static method and return
		this.Size = () => size;
		return size;
	}

	draw(template, civCode)
	{
		this.productionIcon.sprite = "stretched:" + this.page.IconPath + template.icon;
		this.productionIcon.tooltip = EntityBox.compileTooltip(template);
		this.productionIcon.hidden = false;
		EntityBox.setViewerOnPress(this.productionIcon, template.name.internal, civCode);
	}
}
