/**
 * This class handles the loading of files.
 */
class TemplateLoader
{
	constructor()
	{
		/**
		 * Raw Data Caches.
		 */
		this.auraData = {};
		this.technologyData = {};
		this.templateData = {};

		/**
		 * Partly-composed data.
		 */
		this.autoResearchTechList = this.findAllAutoResearchedTechs();
	}

	/**
	 * Loads raw aura template.
	 *
	 * Loads from local cache if available, else from file system.
	 *
	 * @param {string} templateName
	 * @return {Object} Object containing raw template data.
	 */
	loadAuraTemplate(templateName)
	{
		if (!(templateName in this.auraData))
		{
			let data = Engine.ReadJSONFile(this.AuraPath + templateName + ".json");
			translateObjectKeys(data, this.AuraTranslateKeys);

			this.auraData[templateName] = data;
		}

		return this.auraData[templateName];
	}

	/**
	 * Loads raw entity template.
	 *
	 * Loads from local cache if data present, else from file system.
	 *
	 * @param {string} templateName
	 * @param {string} civCode
	 * @return {Object} Object containing raw template data.
	 */
	loadEntityTemplate(templateName, civCode)
	{
		if (!(templateName in this.templateData))
		{
			// We need to clone the template because we want to perform some translations.
			let data = clone(Engine.GetTemplate(templateName));
			translateObjectKeys(data, this.EntityTranslateKeys);

			if (data.Auras)
				for (let auraID of data.Auras._string.split(/\s+/))
					this.loadAuraTemplate(auraID);

			if (data.Identity.Civ != this.DefaultCiv && civCode != this.DefaultCiv && data.Identity.Civ != civCode)
				warn("The \"" + templateName + "\" template has a defined civ of \"" + data.Identity.Civ + "\". " +
					"This does not match the currently selected civ \"" + civCode + "\".");

			this.templateData[templateName] = data;
		}

		return this.templateData[templateName];
	}

	/**
	 * Loads raw technology template.
	 *
	 * Loads from local cache if available, else from file system.
	 *
	 * @param {string} templateName
	 * @return {Object} Object containing raw template data.
	 */
	loadTechnologyTemplate(templateName)
	{
		if (!(templateName in this.technologyData))
		{
			let data = Engine.ReadJSONFile(this.TechnologyPath + templateName + ".json");
			translateObjectKeys(data, this.TechnologyTranslateKeys);

			this.technologyData[templateName] = data;
		}

		return this.technologyData[templateName];
	}

	/**
	 * @param {string} templateName
	 * @param {string} civCode
	 * @return {Object} Contains a list and the requirements of the techs in the pair
	 */
	loadTechnologyPairTemplate(templateName, civCode)
	{
		let template = this.loadTechnologyTemplate(templateName);
		return {
			"techs": [template.top, template.bottom],
			"reqs": DeriveTechnologyRequirements(template, civCode)
		};
	}

	deriveProductionQueue(template, civCode)
	{
		let production = {
			"techs": [],
			"units": []
		};

		if (!template.ProductionQueue)
			return production;

		if (template.ProductionQueue.Entities && template.ProductionQueue.Entities._string)
			for (let templateName of template.ProductionQueue.Entities._string.split(" "))
			{
				templateName = templateName.replace(/\{(civ|native)\}/g, civCode);
				if (Engine.TemplateExists(templateName))
					production.units.push(this.getBaseTemplateName(templateName, civCode));
			}

		let appendTechnology = (technologyName) => {
			let technology = this.loadTechnologyTemplate(technologyName, civCode);
			if (DeriveTechnologyRequirements(technology, civCode))
				production.techs.push(technologyName);
		};

		if (template.ProductionQueue.Technologies && template.ProductionQueue.Technologies._string)
			for (let technologyName of template.ProductionQueue.Technologies._string.split(" "))
			{
				if (technologyName.indexOf("{civ}") != -1)
				{
					let civTechName = technologyName.replace("{civ}", civCode);
					technologyName = TechnologyTemplateExists(civTechName) ? civTechName : technologyName.replace("{civ}", "generic");
				}

				if (this.isPairTech(technologyName))
				{
					let technologyPair = this.loadTechnologyPairTemplate(technologyName, civCode);
					if (technologyPair.reqs)
						for (technologyName of technologyPair.techs)
							appendTechnology(technologyName);
				}
				else
					appendTechnology(technologyName);
			}

		return production;
	}

	deriveBuildQueue(template, civCode)
	{
		let buildQueue = [];

		if (!template.Builder || !template.Builder.Entities._string)
			return buildQueue;

		for (let build of template.Builder.Entities._string.split(" "))
		{
			build = build.replace(/\{(civ|native)\}/g, civCode);
			if (Engine.TemplateExists(build))
				buildQueue.push(build);
		}

		return buildQueue;
	}

	deriveModifications(civCode)
	{
		let techData = [];
		for (let techName of this.autoResearchTechList)
			techData.push(GetTechnologyBasicDataHelper(this.loadTechnologyTemplate(techName), civCode));

		return DeriveModificationsFromTechnologies(techData);
	}

	/**
	 * Crudely iterates through every tech JSON file and identifies those
	 * that are auto-researched.
	 *
	 * @return {array} List of techs that are researched automatically
	 */
	findAllAutoResearchedTechs()
	{
		let techList = [];
		for (let templateName of listFiles(this.TechnologyPath, ".json", true))
		{
			let data = this.loadTechnologyTemplate(templateName);
			if (data && data.autoResearch)
				techList.push(templateName);
		}
		return techList;
	}

	/**
	 * Returns the name of a template's base form (without `_house`, `_trireme`, or similar),
	 * or the template's own name if the base is of a different promotion rank.
	 */
	getBaseTemplateName(templateName, civCode)
	{
		if (!templateName || !Engine.TemplateExists(templateName))
			return undefined;

		templateName = removeFiltersFromTemplateName(templateName);
		let template = this.loadEntityTemplate(templateName, civCode);

		if (!dirname(templateName) || dirname(template["@parent"]) != dirname(templateName))
			return templateName;

		let parentTemplate = this.loadEntityTemplate(template["@parent"], civCode);

		if (parentTemplate.Identity && parentTemplate.Identity.Rank &&
			parentTemplate.Identity.Rank != template.Identity.Rank)
			return templateName;

		if (!parentTemplate.Cost)
			return templateName;

		if (parentTemplate.Upgrade)
			for (let upgrade in parentTemplate.Upgrade)
				if (parentTemplate.Upgrade[upgrade].Entity)
					return templateName;

		for (let res in parentTemplate.Cost.Resources)
			if (+parentTemplate.Cost.Resources[res])
				return this.getBaseTemplateName(template["@parent"], civCode);

		return templateName;
	}

	isPairTech(technologyCode)
	{
		return !!this.loadTechnologyTemplate(technologyCode).top;
	}

	isPhaseTech(technologyCode)
	{
		return basename(technologyCode).startsWith("phase");
	}

}

/**
 * Paths to certain files.
 *
 * It might be nice if we could get these from somewhere, instead of having them hardcoded here.
 */
TemplateLoader.prototype.AuraPath = "simulation/data/auras/";
TemplateLoader.prototype.TechnologyPath = "simulation/data/technologies/";

TemplateLoader.prototype.DefaultCiv = "gaia";

/**
 * Keys of template values that are to be translated on load.
 */
TemplateLoader.prototype.AuraTranslateKeys = ["auraName", "auraDescription"];
TemplateLoader.prototype.EntityTranslateKeys = ["GenericName", "SpecificName", "Tooltip", "History"];
TemplateLoader.prototype.TechnologyTranslateKeys = ["genericName", "tooltip", "description"];
