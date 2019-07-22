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
		translateObjectKeys(data, ["GenericName", "SpecificName", "Tooltip", "History"]);

		if (data.Auras)
			for (let auraID of data.Auras._string.split(/\s+/))
				loadAuraData(auraID);

		if (data.Identity.Civ != "gaia" && g_SelectedCiv != "gaia" && data.Identity.Civ != g_SelectedCiv)
			warn("The \"" + templateName + "\" template has a defined civ of \"" + data.Identity.Civ + "\". " +
				"This does not match the currently selected civ \"" + g_SelectedCiv + "\".");

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
		translateObjectKeys(data, ["genericName", "tooltip", "description"]);

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
 * Load and parse a structure, unit, resource, etc from its entity template file.
 *
 * @return {(object|null)} Sanitized object about the requested template or null if entity template doesn't exist.
 */
function loadEntityTemplate(templateName)
{
	if (!Engine.TemplateExists(templateName))
		return null;

	let template = loadTemplate(templateName);
	let parsed = GetTemplateDataHelper(template, null, g_AuraData, g_ResourceData, g_CurrentModifiers);
	parsed.name.internal = templateName;

	parsed.history = template.Identity.History;

	parsed.production = loadProductionQueue(template);
	if (template.Builder)
		parsed.builder = loadBuildQueue(template);

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
		parsed.upgrades = getActualUpgradeData(parsed.upgrades);

	if (parsed.wallSet)
	{
		parsed.wallset = {};

		if (!parsed.upgrades)
			parsed.upgrades = [];

		// Note: An assumption is made here that wall segments all have the same armor and auras
		let struct = loadEntityTemplate(parsed.wallSet.templates.long);
		parsed.armour = struct.armour;
		parsed.auras = struct.auras;

		// For technology cost multiplier, we need to use the tower
		struct = loadEntityTemplate(parsed.wallSet.templates.tower);
		parsed.techCostMultiplier = struct.techCostMultiplier;

		let health;

		for (let wSegm in parsed.wallSet.templates)
		{
			if (wSegm == "fort" || wSegm == "curves")
				continue;

			let wPart = loadEntityTemplate(parsed.wallSet.templates[wSegm]);
			parsed.wallset[wSegm] = wPart;

			for (let research of wPart.production.techs)
				parsed.production.techs.push(research);

			if (wPart.upgrades)
				parsed.upgrades = parsed.upgrades.concat(wPart.upgrades);

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
				let wPart = loadEntityTemplate(curve);
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

	return parsed;
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
	tech.name.internal = techName;

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
	let phase = loadTechnology(phaseCode);

	phase.actualPhase = phaseCode;
	if (phase.replaces !== undefined)
		phase.actualPhase = phase.replaces[0];

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
