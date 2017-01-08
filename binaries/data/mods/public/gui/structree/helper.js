var g_TemplateData = {};
var g_TechnologyData = {};
var g_AuraData = {};

function loadTemplate(templateName)
{
	if (!(templateName in g_TemplateData))
	{
		// We need to clone the template because we want to perform some translations.
		var data = clone(Engine.GetTemplate(templateName));
		translateObjectKeys(data, ["GenericName", "SpecificName", "Tooltip"]);

		if (data.Auras)
			for (let auraID of data.Auras._string.split(/\s+/))
				loadAuraData(auraID);

		g_TemplateData[templateName] = data;
	}

	return g_TemplateData[templateName];
}

function loadTechData(templateName)
{
	if (!(templateName in g_TechnologyData))
	{
		var filename = "simulation/data/technologies/" + templateName + ".json";
		var data = Engine.ReadJSONFile(filename);
		translateObjectKeys(data, ["genericName", "tooltip"]);

		g_TechnologyData[templateName] = data;
	}

	return g_TechnologyData[templateName];
}

function loadAuraData(templateName)
{
	if (!(templateName in g_AuraData))
	{
		let filename = "simulation/data/auras/" + templateName + ".json";
		let data = Engine.ReadJSONFile(filename);
		translateObjectKeys(data, ["auraName", "auraDescription"]);

		g_AuraData[templateName] = data;
	}

	return g_AuraData[templateName];
}

/**
 * This is needed because getEntityCostTooltip in tooltip.js needs to get
 * the template data of the different wallSet pieces. In the session this
 * function does some caching, but here we do that in loadTemplate already.
 */
function GetTemplateData(templateName)
{
	var template = loadTemplate(templateName);
	return GetTemplateDataHelper(template, null, g_AuraData, g_ResourceData);
}

/**
 * Determines and returns the phase in which a given technology can be
 * first researched. Works recursively through the given tech's
 * pre-requisite and superseded techs if necessary.
 *
 * @param {string} techName - The Technology's name
 * @return The name of the phase the technology belongs to, or false if
 *         the current civ can't research this tech
 */
function GetPhaseOfTechnology(techName)
{
	let phaseIdx = -1;

	if (basename(techName).slice(0, 5) === "phase")
	{
		phaseIdx = g_ParsedData.phaseList.indexOf(GetActualPhase(techName));
		if (phaseIdx > 0)
			return g_ParsedData.phaseList[phaseIdx - 1];
	}

	if (!g_ParsedData.techs[g_SelectedCiv][techName])
		warn(g_SelectedCiv + " : " + techName);
	let techReqs = g_ParsedData.techs[g_SelectedCiv][techName].reqs;
	if (!techReqs)
		return false;

	for (let option of techReqs)
		if (option.techs)
			for (let tech of option.techs)
			{
				if (basename(tech).slice(0, 5) === "phase")
					return tech;
				phaseIdx = Math.max(phaseIdx, g_ParsedData.phaseList.indexOf(GetPhaseOfTechnology(tech)));
			}
	return g_ParsedData.phaseList[phaseIdx] || false;
}

function GetActualPhase(phaseName)
{
	if (g_ParsedData.phases[phaseName])
		return g_ParsedData.phases[phaseName].actualPhase;

	warn("Unrecognised phase (" + techName + ")");
	return g_ParsedData.phaseList[0];
}

function GetPhaseOfTemplate(template)
{
	if (!template.requiredTechnology)
		return g_ParsedData.phaseList[0];

	if (basename(template.requiredTechnology).slice(0, 5) == "phase")
		return GetActualPhase(template.requiredTechnology);

	return GetPhaseOfTechnology(template.requiredTechnology);
}
