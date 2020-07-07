class PhaseIdentManager
{
	constructor(page)
	{
		this.page = page;
		this.idents = [];

		this.PhaseIdents = Engine.GetGUIObjectByName("phaseIdents");
		this.Idents = [];
		for (let identIdx in this.PhaseIdents.children)
			this.Idents.push(new PhaseIdent(this.page, identIdx));
	}

	draw(phaseList, civCode, runningWidths, leftMargin)
	{
		for (let i = 0; i < phaseList.length; ++i)
		{
			let barLength = leftMargin + runningWidths[i] + EntityBox.prototype.HMargin * 0.75;
			this.Idents[i].draw(phaseList, barLength, civCode);
		}
		hideRemaining(this.PhaseIdents.name, phaseList.length);
	}
}
