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
		this.players = {};

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

		let affectedPlayers = template.affectedPlayers || this.AuraAffectedPlayerDefault;
		parsed.affectsTeam = this.AuraTeamIndicators.some(indicator => affectedPlayers.includes(indicator));
		parsed.affectsSelf = this.AuraSelfIndicators.some(indicator => affectedPlayers.includes(indicator));

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
		const parsed = GetTemplateDataHelper(template, null, this.TemplateLoader.auraData, g_ResourceData, this.modifiers[civCode] || {});
		parsed.name.internal = templateName;

		parsed.history = template.Identity.History;

		parsed.production = this.TemplateLoader.deriveProduction(template, civCode);
		if (template.Builder)
			parsed.builder = this.TemplateLoader.deriveBuildQueue(template, civCode);

		// Set the minimum phase that this entity is available.
		// For gaia objects, this is meaningless.
		// Complex requirements are too difficult to process for now, so assume the first phase.
		if (!parsed.requirements?.Techs)
			parsed.phase = this.phaseList[0];
		else
		{
			let highestPhaseIndex = 0;
			for (const tech of parsed.requirements.Techs._string.split(" "))
			{
				if (tech[0] === "!")
					continue;

				const phaseIndex = this.phaseList.indexOf(
					this.TemplateLoader.isPhaseTech(tech) ? this.getActualPhase(tech) :
						this.getPhaseOfTechnology(tech, civCode));
				if (phaseIndex > highestPhaseIndex)
					highestPhaseIndex = phaseIndex;
			}
			parsed.phase = this.phaseList[highestPhaseIndex];
		}

		if (template.Identity.Rank)
			parsed.promotion = {
				"current_rank": template.Identity.Rank,
				"entity": template.Promotion && template.Promotion.Entity
			};

		if (template.ResourceSupply)
			parsed.supply = {
				"type": template.ResourceSupply.Type.split("."),
				"amount": template.ResourceSupply.Max,
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
		const tech = GetTechnologyDataHelper(template, civCode, g_ResourceData, this.modifiers[civCode] || {});
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
	 * Load and parse the relevant player_{civ}.xml template.
	 */
	getPlayer(civCode)
	{
		if (civCode in this.players)
			return this.players[civCode];

		let template = this.TemplateLoader.loadPlayerTemplate(civCode);
		let parsed = {
			"civbonuses": [],
			"teambonuses": [],
		};

		if (template.Auras)
			for (let auraTemplateName of template.Auras._string.split(/\s+/))
				if (AuraTemplateExists(auraTemplateName))
					if (this.getAura(auraTemplateName).affectsTeam)
						parsed.teambonuses.push(auraTemplateName);
					else
						parsed.civbonuses.push(auraTemplateName);

		this.players[civCode] = parsed;
		return parsed;
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

			const data = GetTemplateDataHelper(this.TemplateLoader.loadEntityTemplate(upgrade.entity, civCode), null, this.TemplateLoader.auraData, g_ResourceData, this.modifiers[civCode] || {});
			data.name.internal = upgrade.entity;
			data.cost = upgrade.cost;
			data.icon = upgrade.icon || data.icon;
			data.tooltip = upgrade.tooltip || data.tooltip;
			data.requirements = upgrade.requirements || data.requirements;

			if (!data.requirements?.Techs)
				data.phase = this.phaseList[0];
			else
			{
				let highestPhaseIndex = 0;
				for (const tech of data.requirements.Techs._string.split(" "))
				{
					if (tech[0] === "!")
						continue;

					const phaseIndex = this.phaseList.indexOf(
						this.TemplateLoader.isPhaseTech(tech) ? this.getActualPhase(tech) :
							this.getPhaseOfTechnology(tech, civCode));
					if (phaseIndex > highestPhaseIndex)
						highestPhaseIndex = phaseIndex;
				}
				data.phase = this.phaseList[highestPhaseIndex];
			}

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
		const player = this.getPlayer(civCode);
		const auraList = clone(player.civbonuses);
		for (const bonusname of player.teambonuses)
			if (this.getAura(bonusname).affectsSelf)
				auraList.push(bonusname);

		this.modifiers[civCode] = this.TemplateLoader.deriveModifications(civCode, auraList);
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

// Default affected player token list to use if an aura doesn't explicitly give one.
// Keep in sync with simulation/components/Auras.js
TemplateParser.prototype.AuraAffectedPlayerDefault =
	["Player"];

// List of tokens that, if found in an aura's "affectedPlayers" attribute, indicate
// that the aura applies to team members.
TemplateParser.prototype.AuraTeamIndicators =
	["MutualAlly", "ExclusiveMutualAlly"];

// List of tokens that, if found in an aura's "affectedPlayers" attribute, indicate
// that the aura applies to the aura's owning civ.
TemplateParser.prototype.AuraSelfIndicators =
	["Player", "Ally", "MutualAlly"];
