class ProductionRowManager
{
	constructor(page, guiName, sortByPhase)
	{
		this.page = page;
		this.width = 0;
		this.sortProductionsByPhase = sortByPhase;

		this.productionRows = [];
		for (let row of Engine.GetGUIObjectByName(guiName).children)
			this.productionRows.push(new ProductionRow(this.page, row, this.productionRows.length));
	}

	draw(template, civCode, phaseIdx=0)
	{
		this.width = 0;

		if (this.sortProductionsByPhase)
			for (let r = 0; r < this.page.TemplateParser.phaseList.length; ++r)
				this.productionRows[r].startDraw(this.page.TemplateParser.phaseList.length - phaseIdx);
		else
			this.productionRows[0].startDraw(1);

		// (Want to draw Units before Techs)
		for (let prodType of Object.keys(template.production).reverse())
			for (let prod of template.production[prodType])
			{
				let pIdx = 0;
				switch (prodType)
				{

				case "units":
					prod = this.page.TemplateParser.getEntity(prod, civCode);
					pIdx = this.page.TemplateParser.phaseList.indexOf(prod.phase);
					break;

				case "techs":
					pIdx = this.page.TemplateParser.phaseList.indexOf(this.page.TemplateParser.getPhaseOfTechnology(prod, civCode));
					prod = clone(this.page.TemplateParser.getTechnology(prod, civCode));
					for (let res in template.techCostMultiplier)
						if (prod.cost[res])
							prod.cost[res] *= template.techCostMultiplier[res];
					break;

				default:
					continue;
				}

				let rowIdx = this.sortProductionsByPhase ? Math.max(0, pIdx - phaseIdx) : 0;
				this.productionRows[rowIdx].drawIcon(prod, civCode)
			}

		if (template.upgrades)
			for (let upgrade of template.upgrades)
			{
				let pIdx = 0;
				if (this.phaseSort)
					pIdx = this.page.TemplateParser.phaseList.indexOf(upgrade.phase);
				let rowIdx = Math.max(0, pIdx - phaseIdx);
				this.productionRows[rowIdx].drawIcon(upgrade, civCode);
			}

		if (template.wallset)
			this.productionRows[0].drawIcon(template.wallset.tower, civCode);

		let r = 0;

		// Tell the production rows used we've finished
		if (this.sortProductionsByPhase)
			for (; r < this.page.TemplateParser.phaseList.length; ++r)
				this.width = Math.max(this.width, this.productionRows[r].finishDraw());
		else
			this.width = this.productionRows[r++].finishDraw();

		// Hide any remaining phase rows
		for (; r < this.productionRows.length; ++r)
			this.productionRows[r].hide();
	}
}
