class PhaseIdent
{
	constructor(page, phaseIdx)
	{
		this.page = page;
		this.phaseIdx = +phaseIdx;

		this.Ident = Engine.GetGUIObjectByName("phase[" + this.phaseIdx + "]_ident");
		this.Icon = Engine.GetGUIObjectByName("phase[" + this.phaseIdx + "]_icon");
		this.Bars = Engine.GetGUIObjectByName("phase[" + this.phaseIdx + "]_bars");

		let prodIconSize = ProductionIcon.Size();
		let entityBoxHeight = EntityBox.IconAndCaptionHeight();
		for (let i = 0; i < this.Bars.children.length; ++i)
		{
			let size = this.Bars.children[i].size;
			size.top = entityBoxHeight + prodIconSize.rowHeight * (i + 1);
			size.bottom = entityBoxHeight + prodIconSize.rowHeight * (i + 2) - prodIconSize.rowGap;
			this.Bars.children[i].size = size;
		}
	}

	draw(phaseList, barLength, civCode)
	{
		// Position ident
		let identSize = this.Ident.size;
		identSize.top = TreeSection.getPositionOffset(this.phaseIdx, this.page.TemplateParser);
		identSize.bottom = TreeSection.getPositionOffset(this.phaseIdx + 1, this.page.TemplateParser);
		this.Ident.size = identSize;

		// Draw main icon
		this.drawPhaseIcon(this.Icon, this.phaseIdx, civCode);

		// Draw the phase bars
		let i = 1;
		for (; i < phaseList.length - this.phaseIdx; ++i)
		{
			let prodBar = this.Bars.children[(i - 1)];
			let prodBarSize = prodBar.size;
			prodBarSize.right = barLength;
			prodBar.size = prodBarSize;
			prodBar.hidden = false;

			this.drawPhaseIcon(prodBar.children[0], this.phaseIdx + i, civCode);
		}
		hideRemaining(this.Bars.name, i - 1);
	}

	drawPhaseIcon(phaseIcon, phaseIndex, civCode)
	{
		let phaseName = this.page.TemplateParser.phaseList[phaseIndex];
		let prodPhaseTemplate = this.page.TemplateParser.getTechnology(phaseName + "_" + civCode, civCode) || this.page.TemplateParser.getTechnology(phaseName, civCode);

		phaseIcon.sprite = "stretched:" + this.page.IconPath + prodPhaseTemplate.icon;
		phaseIcon.tooltip = getEntityNamesFormatted(prodPhaseTemplate);
	};

}
