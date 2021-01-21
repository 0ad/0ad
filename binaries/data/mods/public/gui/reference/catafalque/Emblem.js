class Emblem
{
	constructor(page, emblemNum)
	{
		this.page = page;

		this.Emblem = Engine.GetGUIObjectByName("emblem[" + emblemNum + "]");
		this.EmblemImage = this.Emblem.children[0];
		this.EmblemCaption = this.Emblem.children[1];

		let size = this.Emblem.size;
		this.cx = (size.right - size.left) / 2;
		this.cy = (size.bottom - size.top) / 2;
	}

	setCiv(civCode, civData)
	{
		let template = this.page.TemplateParser.getEntity(this.CatafalqueTemplateMethod(civCode), civCode);
		if (!template)
			return false;

		this.EmblemImage.sprite = "stretched:" + civData.Emblem;
		this.EmblemImage.tooltip = getAurasTooltip(template);
		this.EmblemCaption.caption = getEntitySpecificNameFormatted(template);
		this.Emblem.hidden = false;
		return true;
	}

	setPosition(x, y)
	{
		let size = this.Emblem.size;
		size.left = x - this.cx;
		size.right = x + this.cx;
		size.top = y - this.cy;
		size.bottom = y + this.cy;
		this.Emblem.size = size;
	}
}

Emblem.prototype.CatafalqueTemplateMethod =
	civCode => "units/" + civCode + "/catafalque";
