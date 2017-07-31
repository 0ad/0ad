var g_SelectedCiv = "";
var g_CallbackSet = false;

/**
 * Initialises civ data. Not done automatically in case a page doesn't want it.
 */
function init_civs()
{
	g_CivData = loadCivData(true);
}

function closePage()
{
	if (g_CallbackSet)
		Engine.PopGuiPageCB(0);
	else
		Engine.PopGuiPage();
}

/**
 * Compile lists of templates buildable/trainable/researchable of a given civ.
 *
 * @param {string} civCode - Code of the civ to get template list for. Optional,
 *                           defaults to g_SelectedCiv.
 * @return {object} containing lists of template names, grouped by type.
 */
function compileTemplateLists(civCode)
{
	if (!civCode || civCode == "gaia")
		return {};

	let templateLists = {
		"units": new Map(),
		"structures": new Map(),
		"techs": new Map()
	};

	// Get starting entities from {civ}.json
	for (let entity of g_CivData[civCode].StartEntities)
		if (entity.Template.startsWith("units"))
			templateLists.units.set(entity.Template, []);
		else if (entity.Template.startsWith("struct"))
			templateLists.structures.set(entity.Template, []);

	let unitCount = 0;
	do
	{
		unitCount = templateLists.units.size;

		for (let templateName of templateLists.units.keys())
		{
			let newList = getTemplateListsFromUnit(templateName);
			for (let key in newList)
				for (let entity of newList[key])
					if (!templateLists[key].has(entity))
						templateLists[key].set(entity, [templateName]);
					else if (templateLists[key].get(entity).indexOf(templateName) === -1)
						templateLists[key].get(entity).push(templateName);
		}

		for (let templateName of templateLists.structures.keys())
		{
			let newList = getTemplateListsFromStructure(templateName);
			for (let key in newList)
				for (let entity of newList[key])
					if (!templateLists[key].has(entity))
						templateLists[key].set(entity, [templateName]);
					else if (templateLists[key].get(entity).indexOf(templateName) === -1)
						templateLists[key].get(entity).push(templateName);
		}

	} while (unitCount < templateLists.units.size);

	// Expand/filter tech pairs
	for (let [techCode, researcherList] of templateLists.techs)
	{
		if (!isPairTech(techCode))
			continue;

		for (let subTech of loadTechnologyPair(techCode).techs)
			if (!templateLists.techs.has(subTech))
				templateLists.techs.set(subTech, researcherList);
			else
				for (let researcher of researcherList)
					if (templateLists.techs.get(subTech).indexOf(researcher) === -1)
						templateLists.techs.get(subTech).push(researcher);

		templateLists.techs.delete(techCode);
	}

	return templateLists;
}

/**
 * Compiles lists of buildable, trainable, or researchable entities from
 * a named unit template.
 *
 * @param {string} templateName
 */
function getTemplateListsFromUnit(templateName)
{
	if (!templateName || !Engine.TemplateExists(templateName))
		return {};
	let template = loadTemplate(templateName);

	let templateLists = {
		"structures": [],
		"units": [],
		"techs": []
	};

	if (template.Builder && template.Builder.Entities._string)
		for (let build of template.Builder.Entities._string.split(" "))
		{
			build = build.replace("{civ}", g_SelectedCiv);
			if (Engine.TemplateExists(build) && templateLists.structures.indexOf(build) === -1)
				templateLists.structures.push(build);
		}

	if (template.ProductionQueue)
	{
		if (template.ProductionQueue.Entities)
			for (let build of template.ProductionQueue.Entities._string.split(" "))
			{
				build = build.replace("{civ}", g_SelectedCiv);
				if (Engine.TemplateExists(build) && templateLists.units.indexOf(build) === -1)
					templateLists.units.push(build);
			}

		if (template.ProductionQueue.Technologies)
			for (let research of template.ProductionQueue.Technologies._string.split(" "))
				if (templateLists.techs.indexOf(research) === -1)
					templateLists.techs.push(research);
	}

	return templateLists;
}

/**
 * Compiles lists of buildable or researchable entities from a named
 * structure template.
 *
 * @param {string} templateName
 */
function getTemplateListsFromStructure(templateName)
{
	if (!templateName || !Engine.TemplateExists(templateName))
		return {};
	let template = loadTemplate(templateName);

	let templateLists = {
		"techs": [],
		"units": []
	};

	if (template.ProductionQueue)
	{
		if (template.ProductionQueue.Entities && template.ProductionQueue.Entities._string)
			for (let build of template.ProductionQueue.Entities._string.split(" "))
			{
				build = build.replace("{civ}", g_SelectedCiv);
				if (Engine.TemplateExists(build) && templateLists.units.indexOf(build) === -1)
					templateLists.units.push(build);
			}

		if (template.ProductionQueue.Technologies && template.ProductionQueue.Technologies._string)
			for (let research of template.ProductionQueue.Technologies._string.split(" "))
				if (templateLists.techs.indexOf(research) === -1)
					templateLists.techs.push(research);
	}

	if (template.WallSet)
	{
		for (let wSegm in template.WallSet.Templates)
		{
			let wEntities = getTemplateListsFromStructure(template.WallSet.Templates[wSegm]);
			for (let entity of wEntities.techs)
				if (templateLists.techs.indexOf(entity) === -1)
					templateLists.techs.push(entity);
		}
	}

	return templateLists;
}
