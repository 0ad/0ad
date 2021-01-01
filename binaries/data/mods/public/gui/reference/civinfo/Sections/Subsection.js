class Subsection
{
	constructor(page)
	{
		this.page = page;
	}

	getAuraCaptions(auraList, civCode)
	{
		let captions = [];
		for (let auraCode of auraList)
		{
			let aura = this.page.TemplateParser.getAura(auraCode);
			if (!aura.civ || aura.civ != civCode)
				continue;

			captions.push(this.page.formatEntry(
				getEntityNames(aura),
				false,
				getDescriptionTooltip(aura)
			));
		}
		return captions;
	}

	getEntityCaptions(entityList, classList, civCode)
	{
		let captions = [];
		for (let entityCode of entityList)
		{
			// Acquire raw template as we need to compare all classes an entity has, not just the visible ones.
			let template = this.page.TemplateLoader.loadEntityTemplate(entityCode, civCode);
			let classListFull = GetIdentityClasses(template.Identity);
			if (!MatchesClassList(classListFull, classList))
				continue;

			let entity = this.page.TemplateParser.getEntity(entityCode, civCode);
			captions.push(this.page.formatEntry(
				getEntityNames(entity),
				getDescriptionTooltip(entity),
				getEntityTooltip(entity)
			));
		}
		return captions;
	}

	getTechnologyCaptions(technologyList, civCode)
	{
		let captions = [];
		for (let techCode of technologyList)
		{
			let technology = this.page.TemplateParser.getTechnology(techCode, civCode);

			// We deliberately pass an invalid civ code here.
			// If it returns with a value other than false, then
			// we know that this tech can be researched by any civ
			let genericReqs = this.page.TemplateParser.getTechnology(techCode, "anyciv").reqs;

			if (!technology.reqs || genericReqs)
				continue;

			captions.push(this.page.formatEntry(
				getEntityNames(technology),
				getDescriptionTooltip(technology),
				getEntityTooltip(technology)
			));
		}
		return captions;
	}
}
