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

function depath(path)
{
	return path.slice(path.lastIndexOf("/") + 1);
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
