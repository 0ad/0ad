/**
 * This class compiles and stores lists of which templates can be built/trained/researched by other templates.
 */
class TemplateLister
{
	constructor(TemplateLoader)
	{
		this.TemplateLoader = TemplateLoader;
		this.templateLists = new Map();
	}

	/**
	 * Compile lists of templates buildable/trainable/researchable of a given civ.
	 *
	 * @param {Object} civCode
	 * @param {Object} civData - Data defining every civ in the game.
	 */
	compileTemplateLists(civCode, civData)
	{
		if (this.hasTemplateLists(civCode))
			return this.templateLists.get(civCode);

		let templatesToParse = civData[civCode].StartEntities.map(entity => entity.Template);

		let templateLists = {
			"units": new Map(),
			"structures": new Map(),
			"techs": new Map(),
			"wallsetPieces": new Map()
		};

		do
		{
			const templatesThisIteration = templatesToParse;
			templatesToParse = [];

			for (let templateBeingParsed of templatesThisIteration)
			{
				let baseOfTemplateBeingParsed = this.TemplateLoader.getVariantBaseAndType(templateBeingParsed, civCode)[0];
				let list = this.deriveTemplateListsFromTemplate(templateBeingParsed, civCode);
				for (let type in list)
					for (let templateName of list[type])
					{
						if (type != "techs")
						{
							let templateVariance = this.TemplateLoader.getVariantBaseAndType(templateName, civCode);
							if (templateVariance[1].passthru)
								templateName = templateVariance[0];
						}

						if (!templateLists[type].has(templateName))
						{
							templateLists[type].set(templateName, [baseOfTemplateBeingParsed]);
							if (type != "techs")
								templatesToParse.push(templateName);
						}
						else if (templateLists[type].get(templateName).indexOf(baseOfTemplateBeingParsed) == -1)
							templateLists[type].get(templateName).push(baseOfTemplateBeingParsed);
					}
			}
		} while (templatesToParse.length);

		// Expand/filter tech pairs
		for (let [techCode, researcherList] of templateLists.techs)
		{
			if (!this.TemplateLoader.isPairTech(techCode))
				continue;

			for (let subTech of this.TemplateLoader.loadTechnologyPairTemplate(techCode, civCode).techs)
				if (!templateLists.techs.has(subTech))
					templateLists.techs.set(subTech, researcherList);
				else
					for (let researcher of researcherList)
						if (templateLists.techs.get(subTech).indexOf(researcher) == -1)
							templateLists.techs.get(subTech).push(researcher);

			templateLists.techs.delete(techCode);
		}

		// Remove wallset pieces, as they've served their purpose.
		delete templateLists.wallsetPieces;

		this.templateLists.set(civCode, templateLists);
		return this.templateLists.get(civCode);
	}

	/**
	 * Returns a civ's template list.
	 *
	 * Note: this civ must have gone through the compilation process above!
	 *
	 * @param {string} civCode
	 * @return {Object} containing lists of template names, grouped by type.
	 */
	getTemplateLists(civCode)
	{
		if (this.hasTemplateLists(civCode))
			return this.templateLists.get(civCode);

		error("Template lists of \"" + civCode + "\" requested, but this civ has not been loaded.");
		return {};
	}

	/**
	 * Returns whether the civ of the given civCode has been loaded into cache.
	 *
	 * @param {string} civCode
	 * @return {boolean}
	 */
	hasTemplateLists(civCode)
	{
		return this.templateLists.has(civCode);
	}

	/**
	 * Compiles lists of buildable, trainable, or researchable entities from
	 * a named template.
	 */
	deriveTemplateListsFromTemplate(templateName, civCode)
	{
		if (!templateName || !Engine.TemplateExists(templateName))
			return {};

		let template = this.TemplateLoader.loadEntityTemplate(templateName, civCode);

		let templateLists = this.TemplateLoader.deriveProductionQueue(template, civCode);
		templateLists.structures = this.TemplateLoader.deriveBuildQueue(template, civCode);

		if (template.WallSet)
		{
			templateLists.wallsetPieces = [];
			for (let segment in template.WallSet.Templates)
			{
				segment = template.WallSet.Templates[segment].replace(/\{(civ|native)\}/g, civCode);
				if (Engine.TemplateExists(segment))
					templateLists.wallsetPieces.push(segment);
			}
		}

		return templateLists;
	}
}
