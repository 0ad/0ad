var g_SelectedCiv = "gaia";
var g_CallbackSet = false;

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

	let templatesToParse = [];
	for (let entity of g_CivData[civCode].StartEntities)
		templatesToParse.push(entity.Template);

	let templateLists = {
		"units": new Map(),
		"structures": new Map(),
		"techs": new Map(),
		"wallsetPieces": new Map()
	};

	do {
		const templatesThisIteration = templatesToParse;
		templatesToParse = [];

		for (let templateBeingParsed of templatesThisIteration)
		{
			let list = getTemplateListsFromTemplate(templateBeingParsed);
			for (let type in list)
				for (let templateName of list[type])
					if (!templateLists[type].has(templateName))
					{
						templateLists[type].set(templateName, [templateBeingParsed]);
						if (type != "techs")
							templatesToParse.push(templateName);
					}
					else if (templateLists[type].get(templateName).indexOf(templateBeingParsed) == -1)
						templateLists[type].get(templateName).push(templateBeingParsed);
		}
	} while (templatesToParse.length);

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
					if (templateLists.techs.get(subTech).indexOf(researcher) == -1)
						templateLists.techs.get(subTech).push(researcher);

		templateLists.techs.delete(techCode);
	}

	// Remove wallset pieces, as they've served their purpose.
	delete templateLists.wallsetPieces;

	return templateLists;
}

/**
 * Compiles lists of buildable, trainable, or researchable entities from
 * a named template.
 */
function getTemplateListsFromTemplate(templateName)
{
	if (!templateName || !Engine.TemplateExists(templateName))
		return {};

	// If this is a non-promotion variant (ie. {civ}_support_female_citizen_house)
	// then it is functionally equivalent to another unit being processed, so skip it.
	if (getBaseTemplateName(templateName) != templateName)
		return {};

	let template = loadTemplate(templateName);

	let templateLists = loadProductionQueue(template);
	templateLists.structures = loadBuildQueue(template);

	if (template.WallSet)
	{
		templateLists.wallsetPieces = [];
		for (let segment in template.WallSet.Templates)
		{
			segment = template.WallSet.Templates[segment].replace(/\{(civ|native)\}/g, g_SelectedCiv);
			if (Engine.TemplateExists(segment))
				templateLists.wallsetPieces.push(segment);
		}
	}

	return templateLists;
}

function loadProductionQueue(template)
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
			templateName = templateName.replace(/\{(civ|native)\}/g, g_SelectedCiv);
			if (Engine.TemplateExists(templateName))
				production.units.push(getBaseTemplateName(templateName));
		}

	if (template.ProductionQueue.Technologies && template.ProductionQueue.Technologies._string)
		for (let technologyName of template.ProductionQueue.Technologies._string.split(" "))
		{
			if (technologyName.indexOf("{civ}") != -1)
			{
				let civTechName = technologyName.replace("{civ}", g_SelectedCiv);
				technologyName = techDataExists(civTechName) ? civTechName : technologyName.replace("{civ}", "generic");
			}

			if (isPairTech(technologyName))
				for (let pairTechnologyName of loadTechnologyPair(technologyName).techs)
					production.techs.push(pairTechnologyName);
			else
				production.techs.push(technologyName);
		}

	return production;
}

function loadBuildQueue(template)
{
	let buildQueue = [];

	if (!template.Builder || !template.Builder.Entities._string)
		return buildQueue;

	for (let build of template.Builder.Entities._string.split(" "))
	{
		build = build.replace(/\{(civ|native)\}/g, g_SelectedCiv);
		if (Engine.TemplateExists(build))
			buildQueue.push(build);
	}

	return buildQueue;
}

/**
 * Returns the name of a template's base form (without `_house`, `_trireme`, or similar),
 * or the template's own name if the base is of a different promotion rank.
 */
function getBaseTemplateName(templateName)
{
	if (!templateName || !Engine.TemplateExists(templateName))
		return undefined;

	templateName = removeFiltersFromTemplateName(templateName);
	let template = loadTemplate(templateName);

	if (!dirname(templateName) || dirname(template["@parent"]) != dirname(templateName))
		return templateName;

	let parentTemplate = loadTemplate(template["@parent"]);

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
			return getBaseTemplateName(template["@parent"]);

	return templateName;
}

function setViewerOnPress(guiObjectName, templateName)
{
	let viewerFunc = () => {
		Engine.PushGuiPage("page_viewer.xml", {
			"templateName": templateName,
			"civ": g_SelectedCiv
		});
	};
	Engine.GetGUIObjectByName(guiObjectName).onPress = viewerFunc;
	Engine.GetGUIObjectByName(guiObjectName).onPressRight = viewerFunc;
}
