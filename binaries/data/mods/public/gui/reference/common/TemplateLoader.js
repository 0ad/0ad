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
		this.playerData = {};
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
	 * Loads raw player template.
	 *
	 * Loads from local cache if data present, else from file system.
	 *
	 * If a civ doesn't have their own civ-specific template,
	 * then we return the generic template.
	 *
	 * @param {string} civCode
	 * @return {Object} Object containing raw template data.
	 */
	loadPlayerTemplate(civCode)
	{
		if (!(civCode in this.playerData))
		{
			let templateName = this.buildPlayerTemplateName(civCode);
			this.playerData[civCode] = Engine.GetTemplate(templateName);

			// No object keys need to be translated
		}

		return this.playerData[civCode];
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

			// Translate specificName as in GetTechnologyData() from gui/session/session.js
			if (typeof (data.specificName) === 'object')
				for (let civ in data.specificName)
					data.specificName[civ] = translate(data.specificName[civ]);
			else if (data.specificName)
				warn("specificName should be an object of civ->name mappings in " + templateName + ".json");

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

	deriveProduction(template, civCode)
	{
		const production = {
			"techs": [],
			"units": []
		};

		if (!template.Researcher && !template.Trainer)
			return production;

		if (template.Trainer?.Entities?._string)
			for (let templateName of template.Trainer.Entities._string.split(" "))
			{
				templateName = templateName.replace(/\{(civ|native)\}/g, civCode);
				if (Engine.TemplateExists(templateName))
					production.units.push(templateName);
			}

		const appendTechnology = (technologyName) => {
			const technology = this.loadTechnologyTemplate(technologyName, civCode);
			if (DeriveTechnologyRequirements(technology, civCode))
				production.techs.push(technologyName);
		};

		if (template.Researcher?.Technologies?._string)
			for (let technologyName of template.Researcher.Technologies._string.split(" "))
			{
				if (technologyName.indexOf("{civ}") != -1)
				{
					const civTechName = technologyName.replace("{civ}", civCode);
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

	deriveModifications(civCode, auraList)
	{
		const modificationData = [];
		for (const techName of this.autoResearchTechList)
			modificationData.push(GetTechnologyBasicDataHelper(this.loadTechnologyTemplate(techName), civCode));

		for (const auraName of auraList)
			modificationData.push(this.loadAuraTemplate(auraName));

		return DeriveModificationsFromTechnologies(modificationData);
	}

	/**
	 * If a civ doesn't have its own civ-specific player template,
	 * this returns the name of the generic player template.
	 *
	 * @see simulation/helpers/Player.js GetPlayerTemplateName()
	 *      (Which can't be combined with this due to different Engine contexts)
	 */
	buildPlayerTemplateName(civCode)
	{
		let templateName = this.PlayerPath + civCode;
		if (Engine.TemplateExists(templateName))
			return templateName;

		warn("No template found for civ " + civCode + ".");

		return this.PlayerPath + this.DefaultCiv;
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
	 * A template may be a variant of another template,
	 * eg. `*_house`, `*_trireme`, or a promotion.
	 *
	 * This method returns an array containing:
	 * [0] - The template's basename
	 * [1] - The variant type
	 * [2] - Further information (if available)
	 *
	 * e.g.:
	 * units/athen/infantry_swordsman_e
	 *    -> ["units/athen/infantry_swordsman_b", TemplateVariant.promotion, "elite"]
	 *
	 * units/brit/support_female_citizen_house
	 *    -> ["units/brit/support_female_citizen", TemplateVariant.unlockedByTechnology, "unlock_female_house"]
	 */
	getVariantBaseAndType(templateName, civCode)
	{
		if (!templateName || !Engine.TemplateExists(templateName))
			return undefined;

		templateName = removeFiltersFromTemplateName(templateName);
		let template = this.loadEntityTemplate(templateName, civCode);

		if (!dirname(templateName) || dirname(template["@parent"]) != dirname(templateName))
			return [templateName, TemplateVariant.base];

		let parentTemplate = this.loadEntityTemplate(template["@parent"], civCode);
		let inheritedVariance = this.getVariantBaseAndType(template["@parent"], civCode);

		if (parentTemplate.Identity)
		{
			if (parentTemplate.Identity.Civ && parentTemplate.Identity.Civ != template.Identity.Civ)
				return [templateName, TemplateVariant.base];

			if (parentTemplate.Identity.Rank && parentTemplate.Identity.Rank != template.Identity.Rank)
				return [inheritedVariance[0], TemplateVariant.promotion, template.Identity.Rank.toLowerCase()];
		}

		if (parentTemplate.Upgrade)
			for (let upgrade in parentTemplate.Upgrade)
				if (parentTemplate.Upgrade[upgrade].Entity)
					return [inheritedVariance[0], TemplateVariant.upgrade, upgrade.toLowerCase()];

		if (template.Identity.RequiredTechnology)
			return [inheritedVariance[0], TemplateVariant.unlockedByTechnology, template.Identity.RequiredTechnology];

		if (parentTemplate.Cost)
			for (let res in parentTemplate.Cost.Resources)
				if (+parentTemplate.Cost.Resources[res])
					return [inheritedVariance[0], TemplateVariant.trainable];

		warn("Template variance unknown: " + templateName);
		return [templateName, TemplateVariant.unknown];
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
TemplateLoader.prototype.PlayerPath = "special/players/";
TemplateLoader.prototype.TechnologyPath = "simulation/data/technologies/";

TemplateLoader.prototype.DefaultCiv = "gaia";

/**
 * Keys of template values that are to be translated on load.
 */
TemplateLoader.prototype.AuraTranslateKeys = ["auraName", "auraDescription"];
TemplateLoader.prototype.EntityTranslateKeys = ["GenericName", "SpecificName", "Tooltip", "History"];
TemplateLoader.prototype.TechnologyTranslateKeys = ["genericName", "tooltip", "description"];
