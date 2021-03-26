/**
 * Class inherited by StructureBox and TrainerBox classes.
 */
class EntityBox
{
	constructor(page)
	{
		this.page = page;
	}

	static setViewerOnPress(guiObject, templateName, civCode)
	{
		let viewerFunc = () => {
			Engine.PushGuiPage("page_viewer.xml", {
				"templateName": templateName,
				"civ": civCode
			});
		};
		guiObject.onPress = viewerFunc;
		guiObject.onPressRight = viewerFunc;
	}

	draw(templateName, civCode)
	{
		this.template = this.page.TemplateParser.getEntity(templateName, civCode);
		this.gui.hidden = false;

		let caption = this.gui.children[0];
		caption.caption = g_SpecificNamesPrimary ?
			translate(this.template.name.specific) :
			translate(this.template.name.generic);

		let icon = this.gui.children[1];
		icon.sprite = "stretched:" + this.page.IconPath + this.template.icon;
		icon.tooltip = this.constructor.compileTooltip(this.template);
		this.constructor.setViewerOnPress(icon, this.template.name.internal, civCode);
	}

	captionWidth()
	{
		// We make the assumption that the caption's padding is equal on both sides
		let caption = this.gui.children[0];
		return Engine.GetTextWidth(caption.font, caption.caption) + (caption.size.left + caption.buffer_zone) * 2;
	}

	static compileTooltip(template)
	{
		return ReferencePage.buildText(template, this.prototype.TooltipFunctions) + "\n" + showTemplateViewerOnClickTooltip();
	}

	/**
	 * Returns the height between the top of the EntityBox, and the top of the production rows.
	 *
	 * Used within the TreeSection class to position the production rows,
	 * and used with the PhaseIdent class to position grey bars under them.
	 */
	static IconAndCaptionHeight()
	{
		let height = Engine.GetGUIObjectByName("structure[0]_icon").size.bottom + this.prototype.IconPadding;

		// Replace function so the above is only run once.
		this.IconAndCaptionHeight = () => height;
		return height;
	}

}

/**
 * Minimum width of the boxes, the margins between them (Horizontally and Vertically),
 * and the padding between the main icon and the production row(s) beneath it.
 */
EntityBox.prototype.MinWidth = 96;
EntityBox.prototype.HMargin = 8;
EntityBox.prototype.VMargin = 12;
EntityBox.prototype.IconPadding = 8;

/**
 * Functions used to collate the contents of a tooltip.
 */
EntityBox.prototype.TooltipFunctions = [
	getEntityNamesFormatted,
	getEntityCostTooltip,
	getEntityTooltip,
	getAurasTooltip
].concat(ReferencePage.prototype.StatsFunctions);
