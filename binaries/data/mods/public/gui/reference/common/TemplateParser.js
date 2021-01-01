/**
 * This class parses and stores parsed template data.
 */
class TemplateParser
{
	constructor(TemplateLoader)
	{
		this.TemplateLoader = TemplateLoader;

		/**
		 * Parsed Data Stores
		 */
		this.auras = {};
		this.entities = {};
		this.techs = {};
		this.phases = {};
		this.modifiers = {};

		this.phaseList = [];
	}

	getAura(auraName)
	{
		if (auraName in this.auras)
			return this.auras[auraName];

		if (!AuraTemplateExists(auraName))
			return null;

		let template = this.TemplateLoader.loadAuraTemplate(auraName);
		let parsed = GetAuraDataHelper(template);

		if (template.civ)
			parsed.civ = template.civ;

		this.auras[auraName] = parsed;
		return this.auras[auraName];
	}

	/**
	 * Load and parse a structure, unit, resource, etc from its entity template file.
	 *
	 * @param {string} templateName
	 * @param {string} civCode
	 * @return {(object|null)} Sanitized object about the requested template or null if entity template doesn't exist.
	 */
	getEntity(templateName, civCode)
	{
		if (!(civCode in this.entities))
			this.entities[civCode] = {};
		else if (templateName in this.entities[civCode])
			return this.entities[civCode][templateName];

		if (!Engine.TemplateExists(templateName))
			return null;

		let template = this.TemplateLoader.loadEntityTemplate(templateName, civCode);
		let parsed = GetTemplateDataHelper(template, null, this.TemplateLoader.auraData, this.modifiers[civCode] || {});
		parsed.name.internal = templateName;

		parsed.history = template.Identity.History;

		parsed.production = this.TemplateLoader.deriveProductionQueue(template, civCode);
		if (template.Builder)
			parsed.builder = this.TemplateLoader.deriveBuildQueue(template, civCode);

		// Set the minimum phase that this entity is available.
		// For gaia objects, this is meaningless.
		if (!parsed.requiredTechnology)
			parsed.phase = this.phaseList[0];
		else if (this.TemplateLoader.isPhaseTech(parsed.requiredTechnology))
			parsed.phase = this.getActualPhase(parsed.requiredTechnology);
		else
			parsed.phase = this.getPhaseOfTechnology(parsed.requiredTechnology, civCode);

		if (template.Identity.Rank)
			parsed.promotion = {
				"current_rank": template.Identity.Rank,
				"entity": template.Promotion && template.Promotion.Entity
			};

		if (template.ResourceSupply)
			parsed.supply = {
				"type": template.ResourceSupply.Type.split("."),
				"amount": template.ResourceSupply.Amount,
			};

		if (parsed.upgrades)
			parsed.upgrades = this.getActualUpgradeData(parsed.upgrades, civCode);

		if (parsed.wallSet)
		{
			parsed.wallset = {};

			if (!parsed.upgrades)
				parsed.upgrades = [];

			// Note: An assumption is made here that wall segments all have the same resistance and auras
			let struct = this.getEntity(parsed.wallSet.templates.long, civCode);
			parsed.resistance = struct.resistance;
			parsed.auras = struct.auras;

			// For technology cost multiplier, we need to use the tower
			struct = this.getEntity(parsed.wallSet.templates.tower, civCode);
			parsed.techCostMultiplier = struct.techCostMultiplier;

			let health;

			for (let wSegm in parsed.wallSet.templates)
			{
				if (wSegm == "fort" || wSegm == "curves")
					continue;

				let wPart = this.getEntity(parsed.wallSet.templates[wSegm], civCode);
				parsed.wallset[wSegm] = wPart;

				for (let research of wPart.production.techs)
					parsed.production.techs.push(research);

				if (wPart.upgrades)
					Array.prototype.push.apply(parsed.upgrades, wPart.upgrades);

				if (["gate", "tower"].indexOf(wSegm) != -1)
					continue;

				if (!health)
				{
					health = { "min": wPart.health, "max": wPart.health };
					continue;
				}

				health.min = Math.min(health.min, wPart.health);
				health.max = Math.max(health.max, wPart.health);
			}

			if (parsed.wallSet.templates.curves)
				for (let curve of parsed.wallSet.templates.curves)
				{
					let wPart = this.getEntity(curve, civCode);
					health.min = Math.min(health.min, wPart.health);
					health.max = Math.max(health.max, wPart.health);
				}

			if (health.min == health.max)
				parsed.health = health.min;
			else
				parsed.health = sprintf(translate("%(health_min)s to %(health_max)s"), {
					"health_min": health.min,
					"health_max": health.max
				});
		}

		this.entities[civCode][templateName] = parsed;
		return parsed;
	}

	/**
	 * Load and parse technology from json template.
	 *
	 * @param {string} technologyName
	 * @param {string} civCode
	 * @return {Object} Sanitized data about the requested technology.
	 */
	getTechnology(technologyName, civCode)
	{
		if (!TechnologyTemplateExists(technologyName))
			return null;

		if (this.TemplateLoader.isPhaseTech(technologyName) && technologyName in this.phases)
			return this.phases[technologyName];

		if (!(civCode in this.techs))
			this.techs[civCode] = {};
		else if (technologyName in this.techs[civCode])
			return this.techs[civCode][technologyName];

		let template = this.TemplateLoader.loadTechnologyTemplate(technologyName);
		let tech = GetTechnologyDataHelper(template, civCode, g_ResourceData);
		tech.name.internal = technologyName;

		if (template.pair !== undefined)
		{
			tech.pair = template.pair;
			tech.reqs = this.mergeRequirements(tech.reqs, this.TemplateLoader.loadTechnologyPairTemplate(template.pair).reqs);
		}

		if (this.TemplateLoader.isPhaseTech(technologyName))
		{
			tech.actualPhase = technologyName;
			if (tech.replaces !== undefined)
				tech.actualPhase = tech.replaces[0];
			this.phases[technologyName] = tech;
		}
		else
			this.techs[civCode][technologyName] = tech;
		return tech;
	}

	/**
	 * @param {string} phaseCode
	 * @param {string} civCode
	 * @return {Object} Sanitized object containing phase data
	 */
	getPhase(phaseCode, civCode)
	{
		return this.getTechnology(phaseCode, civCode);
	}

	/**
	 * Provided with an array containing basic information about possible
	 * upgrades, such as that generated by globalscript's GetTemplateDataHelper,
	 * this function loads the actual template data of the upgrades, overwrites
	 * certain values within, then passes an array containing the template data
	 * back to caller.
	 */
	getActualUpgradeData(upgradesInfo, civCode)
	{
		let newUpgrades = [];
		for (let upgrade of upgradesInfo)
		{
			upgrade.entity = upgrade.entity.replace(/\{(civ|native)\}/g, civCode);

			let data = GetTemplateDataHelper(this.TemplateLoader.loadEntityTemplate(upgrade.entity, civCode), null, this.TemplateLoader.auraData);
			data.name.internal = upgrade.entity;
			data.cost = upgrade.cost;
			data.icon = upgrade.icon || data.icon;
			data.tooltip = upgrade.tooltip || data.tooltip;
			data.requiredTechnology = upgrade.requiredTechnology || data.requiredTechnology;

			if (!data.requiredTechnology)
				data.phase = this.phaseList[0];
			else if (this.TemplateLoader.isPhaseTech(data.requiredTechnology))
				data.phase = this.getActualPhase(data.requiredTechnology);
			else
				data.phase = this.getPhaseOfTechnology(data.requiredTechnology, civCode);

			newUpgrades.push(data);
		}
		return newUpgrades;
	}

	/**
	 * Determines and returns the phase in which a given technology can be
	 * first researched. Works recursively through the given tech's
	 * pre-requisite and superseded techs if necessary.
	 *
	 * @param {string} techName - The Technology's name
	 * @param {string} civCode
	 * @return The name of the phase the technology belongs to, or false if
	 *         the current civ can't research this tech
	 */
	getPhaseOfTechnology(techName, civCode)
	{
		let phaseIdx = -1;

		if (basename(techName).startsWith("phase"))
		{
			if (!this.phases[techName].reqs)
				return false;

			phaseIdx = this.phaseList.indexOf(this.getActualPhase(techName));
			if (phaseIdx > 0)
				return this.phaseList[phaseIdx - 1];
		}

		let techReqs = this.getTechnology(techName, civCode).reqs;
		if (!techReqs)
			return false;

		for (let option of techReqs)
			if (option.techs)
				for (let tech of option.techs)
				{
					if (basename(tech).startsWith("phase"))
						return tech;
					if (basename(tech).startsWith("pair"))
						continue;
					phaseIdx = Math.max(phaseIdx, this.phaseList.indexOf(this.getPhaseOfTechnology(tech, civCode)));
				}
		return this.phaseList[phaseIdx] || false;
	}

	/**
	 * Returns the actual phase a certain phase tech represents or stands in for.
	 *
	 * For example, passing `phase_city_athen` would result in `phase_city`.
	 *
	 * @param {string} phaseName
	 * @return {string}
	 */
	getActualPhase(phaseName)
	{
		if (this.phases[phaseName])
			return this.phases[phaseName].actualPhase;

		warn("Unrecognized phase (" + phaseName + ")");
		return this.phaseList[0];
	}

	getModifiers(civCode)
	{
		return this.modifiers[civCode];
	}

	deriveModifications(civCode)
	{
		this.modifiers[civCode] = this.TemplateLoader.deriveModifications(civCode);
	}

	derivePhaseList(technologyList, civCode)
	{
		// Load all of a civ's specific phase technologies
		for (let techcode of technologyList)
			if (this.TemplateLoader.isPhaseTech(techcode))
				this.getTechnology(techcode, civCode);

		this.phaseList = UnravelPhases(this.phases);

		// Make sure all required generic phases are loaded and parsed
		for (let phasecode of this.phaseList)
			this.getTechnology(phasecode, civCode);
	}

	mergeRequirements(reqsA, reqsB)
	{
		if (!reqsA || !reqsB)
			return false;

		let finalReqs = clone(reqsA);

		for (let option of reqsB)
			for (let type in option)
				for (let opt in finalReqs)
				{
					if (!finalReqs[opt][type])
						finalReqs[opt][type] = [];
					Array.prototype.push.apply(finalReqs[opt][type], option[type]);
				}

		return finalReqs;
	}
}
