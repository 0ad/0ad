var g_TemplateData = {};
var g_TechnologyData = {};

function loadTemplate(templateName)
{
	if (!(templateName in g_TemplateData))
	{
		// We need to clone the template because we want to perform some translations.
		var data = clone(Engine.GetTemplate(templateName));
		translateObjectKeys(data, ["GenericName", "SpecificName", "Tooltip"]);
		
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

/**
 * Fetch a value from an entity's template
 *
 * @param templateName The template to retreive the value from
 * @param keypath The path to the value to be fetched. "Identity/GenericName"
 *                is equivalent to {"Identity":{"GenericName":"FOOBAR"}}
 *
 * @return The content requested at the key-path defined, or a blank array if
 *           not found
 */
function fetchValue(templateName, keypath)
{
	var keys = keypath.split("/");
	var template = loadTemplate(templateName);

	let k = 0;
	for (; k < keys.length-1; ++k)
	{
		if (template[keys[k]] === undefined)
			return [];

		template = template[keys[k]];
	}
	if (template[keys[k]] === undefined)
		return [];

	return template[keys[k]];
}

/**
 * Fetch tokens from an entity's template
 * @return An array containing all tokens if found, else an empty array
 * @see fetchValue
 */
function fetchTokens(templateName, keypath)
{
	var val = fetchValue(templateName, keypath);
	if (!("_string" in val))
		return [];

	return val._string.split(" ");
}

function depath(path)
{
	return path.slice(path.lastIndexOf("/")+1);
}

/**
 * This is needed because getEntityCostTooltip in tooltip.js needs to get
 * the template data of the different wallSet pieces. In the session this
 * function does some caching, but here we do that in loadTemplate already.
 */
function GetTemplateData(templateName)
{
	var template = loadTemplate(templateName);
	return GetTemplateDataHelper(template);
}
