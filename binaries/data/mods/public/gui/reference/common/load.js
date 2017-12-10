/**
 * Paths to certain files.
 */
const g_TechnologyPath = "simulation/data/technologies/";
const g_AuraPath = "simulation/data/auras/";

/**
 * Raw Data Caches.
 */
var g_AuraData = {};
var g_TemplateData = {};
var g_TechnologyData = {};
var g_CivData = loadCivData(true, false);

/**
 * Parsed Data Stores.
 */
var g_ParsedData = {};
var g_ResourceData = new Resources();
var g_DamageTypes = new DamageTypes();

// This must be defined after the g_TechnologyData cache object is declared.
var g_AutoResearchTechList = findAllAutoResearchedTechs();

/**
 * Loads raw entity template.
 *
 * Loads from local cache if data present, else from file system.
 *
 * @param {string} templateName
 * @return {object} Object containing raw template data.
 */
function loadTemplate(templateName)
{
	if (!(templateName in g_TemplateData))
	{
		// We need to clone the template because we want to perform some translations.
		let data = clone(Engine.GetTemplate(templateName));
		translateObjectKeys(data, ["GenericName", "SpecificName", "Tooltip"]);

		if (data.Auras)
			for (let auraID of data.Auras._string.split(/\s+/))
				loadAuraData(auraID);

		g_TemplateData[templateName] = data;
	}

	return g_TemplateData[templateName];
}

/**
 * Loads raw technology template.
 *
 * Loads from local cache if available, else from file system.
 *
 * @param {string} templateName
 * @return {object} Object containing raw template data.
 */
function loadTechData(templateName)
{
	if (!(templateName in g_TechnologyData))
	{
		let data = Engine.ReadJSONFile(g_TechnologyPath + templateName + ".json");
		translateObjectKeys(data, ["genericName", "tooltip"]);

		g_TechnologyData[templateName] = data;
	}

	return g_TechnologyData[templateName];
}

function techDataExists(templateName)
{
	return Engine.FileExists("simulation/data/technologies/" + templateName + ".json");
}

/**
 * Loads raw aura template.
 *
 * Loads from local cache if available, else from file system.
 *
 * @param {string} templateName
 * @return {object} Object containing raw template data.
 */
function loadAuraData(templateName)
{
	if (!(templateName in g_AuraData))
	{
		let data = Engine.ReadJSONFile(g_AuraPath + templateName + ".json");
		translateObjectKeys(data, ["auraName", "auraDescription"]);

		g_AuraData[templateName] = data;
	}

	return g_AuraData[templateName];
}

/**
 * Load and parse unit from entity template.
 *
 * @param {string} templateName
 * @return Sanitized object about the requested unit or null if entity template doesn't exist.
 */
function loadUnit(templateName)
{
	if (!Engine.TemplateExists(templateName))
		return null;

	let template = loadTemplate(templateName);
	let unit = GetTemplateDataHelper(template, null, g_AuraData, g_ResourceData, g_DamageTypes, g_CurrentModifiers);

	if (template.ProductionQueue)
	{
		unit.production = {};
		if (template.ProductionQueue.Entities)
		{
			unit.production.units = [];
			for (let build of template.ProductionQueue.Entities._string.split(" "))
			{
				build = build.replace(/\{(civ|native)\}/g, g_SelectedCiv);
				if (Engine.TemplateExists(build))
					unit.production.units.push(build);
			}
		}
		if (template.ProductionQueue.Technologies)
		{
			unit.production.techs = [];
			for (let research of template.ProductionQueue.Technologies._string.split(" "))
			{
				if (research.indexOf("{civ}") != -1)
				{
					let civResearch = research.replace("{civ}", g_SelectedCiv);
					research = techDataExists(civResearch) ?
					           civResearch : research.replace("{civ}", "generic");
				}
				if (isPairTech(research))
					for (let tech of loadTechnologyPair(research).techs)
						unit.production.techs.push(tech);
				else
					unit.production.techs.push(research);
			}
		}
	}

	if (template.Builder && template.Builder.Entities._string)
	{
		unit.builder = [];
		for (let build of template.Builder.Entities._string.split(" "))
		{
			build = build.replace(/\{(civ|native)\}/g, g_SelectedCiv);
			if (Engine.TemplateExists(build))
				unit.builder.push(build);
		}
	}

	if (unit.upgrades)
		unit.upgrades = getActualUpgradeData(unit.upgrades);

	return unit;
}

/**
 * Load and parse structure from entity template.
 *
 * @param {string} templateName
 * @return {object} Sanitized data about the requested structure or null if entity template doesn't exist.
 */
function loadStructure(templateName)
{
	if (!Engine.TemplateExists(templateName))
		return null;

	let template = loadTemplate(templateName);
	let structure = GetTemplateDataHelper(template, null, g_AuraData, g_ResourceData, g_DamageTypes, g_CurrentModifiers);

	structure.production = {
		"technology": [],
		"units": []
	};

	if (template.ProductionQueue)
	{
		if (template.ProductionQueue.Entities && template.ProductionQueue.Entities._string)
			for (let build of template.ProductionQueue.Entities._string.split(" "))
			{
				build = build.replace(/\{(civ|native)\}/g, g_SelectedCiv);
				if (Engine.TemplateExists(build))
					structure.production.units.push(build);
			}

		if (template.ProductionQueue.Technologies && template.ProductionQueue.Technologies._string)
			for (let research of template.ProductionQueue.Technologies._string.split(" "))
			{
				if (research.indexOf("{civ}") != -1)
				{
					let civResearch = research.replace("{civ}", g_SelectedCiv);
					research = techDataExists(civResearch) ?
					           civResearch : research.replace("{civ}", "generic");
				}
				if (isPairTech(research))
					for (let tech of loadTechnologyPair(research).techs)
						structure.production.technology.push(tech);
				else
					structure.production.technology.push(research);
			}
	}

	if (structure.upgrades)
		structure.upgrades = getActualUpgradeData(structure.upgrades);

	if (structure.wallSet)
	{
		structure.wallset = {};

		if (!structure.upgrades)
			structure.upgrades = [];

		// Note: An assumption is made here that wall segments all have the same armor and auras
		let struct = loadStructure(structure.wallSet.templates.long);
		structure.armour = struct.armour;
		structure.auras = struct.auras;

		// For technology cost multiplier, we need to use the tower
		struct = loadStructure(structure.wallSet.templates.tower);
		structure.techCostMultiplier = struct.techCostMultiplier;

		let health;

		for (let wSegm in structure.wallSet.templates)
		{
			if (wSegm == "fort" || wSegm == "curves")
				continue;

			let wPart = loadStructure(structure.wallSet.templates[wSegm]);
			structure.wallset[wSegm] = wPart;

			for (let research of wPart.production.technology)
				structure.production.technology.push(research);

			if (wPart.upgrades)
				structure.upgrades = structure.upgrades.concat(wPart.upgrades);

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

		if (structure.wallSet.templates.curves)
			for (let curve of structure.wallSet.templates.curves)
			{
				let wPart = loadStructure(curve);
				health.min = Math.min(health.min, wPart.health);
				health.max = Math.max(health.max, wPart.health);
			}

		if (health.min == health.max)
			structure.health = health.min;
		else
			structure.health = sprintf(translate("%(health_min)s to %(health_max)s"), {
				"health_min": health.min,
				"health_max": health.max
			});
	}

	return structure;
}

/**
 * Load and parse technology from json template.
 *
 * @param {string} templateName
 * @return {object} Sanitized data about the requested technology.
 */
function loadTechnology(techName)
{
	let template = loadTechData(techName);
	let tech = GetTechnologyDataHelper(template, g_SelectedCiv, g_ResourceData);

	if (template.pair !== undefined)
	{
		tech.pair = template.pair;
		tech.reqs = mergeRequirements(tech.reqs, loadTechnologyPair(template.pair).reqs);
	}

	return tech;
}

/**
 * Crudely iterates through every tech JSON file and identifies those
 * that are auto-researched.
 *
 * @return {array} List of techs that are researched automatically
 */
function findAllAutoResearchedTechs()
{
	let techList = [];

	for (let filename of Engine.ListDirectoryFiles(g_TechnologyPath, "*.json", true))
	{
		// -5 to strip off the file extension
		let templateName = filename.slice(g_TechnologyPath.length, -5);
		let data = loadTechData(templateName);

		if (data && data.autoResearch)
			techList.push(templateName);
	}

	return techList;
}

/**
 * @param {string} phaseCode
 * @return {object} Sanitized object containing phase data
 */
function loadPhase(phaseCode)
{
	let template = loadTechData(phaseCode);
	let phase = loadTechnology(phaseCode);

	phase.actualPhase = phaseCode;
	if (template.replaces !== undefined)
		phase.actualPhase = template.replaces[0];

	return phase;
}

/**
 * @param {string} pairCode
 * @return {object} Contains a list and the requirements of the techs in the pair
 */
function loadTechnologyPair(pairCode)
{
	var pairInfo = loadTechData(pairCode);

	return {
		"techs": [ pairInfo.top, pairInfo.bottom ],
		"reqs": DeriveTechnologyRequirements(pairInfo, g_SelectedCiv)
	};
}

/**
 * @param {string} modCode
 * @return {object} Sanitized object containing modifier tech data
 */
function loadModifierTech(modCode)
{
	if (!Engine.FileExists("simulation/data/technologies/"+modCode+".json"))
		return {};
	return loadTechData(modCode);
}
