/**
 * Array of structure template names when given a civ and a phase name.
 */
var g_BuildList = {};

/**
 * Array of template names that can be trained from a unit, given a civ and unit template name.  
 */
var g_TrainList = {};

/**
 * Initialize the page
 *
 * @param {object} data - Parameters passed from the code that calls this page into existence.
 */
function init(data = {})
{
	if (data.callback)
		g_CallbackSet = true;

	let civList = Object.keys(g_CivData).map(civ => ({
		"name": g_CivData[civ].Name,
		"code": civ,
	})).sort(sortNameIgnoreCase);

	if (!civList.length)
	{
		closePage();
		return;
	}

	g_ParsedData = {
		"units": {},
		"structures": {},
		"techs": {},
		"phases": {}
	};

	let civSelection = Engine.GetGUIObjectByName("civSelection");
	civSelection.list = civList.map(c => c.name);
	civSelection.list_data = civList.map(c => c.code);
	civSelection.selected = data.civ ? civSelection.list_data.indexOf(data.civ) : 0;
}

/**
 * @param {string} civCode
 */
function selectCiv(civCode)
{
	if (civCode === g_SelectedCiv || !g_CivData[civCode])
		return;

	g_SelectedCiv = civCode;

	g_CurrentModifiers = deriveModifications(g_AutoResearchTechList);

	// If a buildList already exists, then this civ has already been parsed
	if (g_BuildList[g_SelectedCiv])
	{
		draw();
		drawPhaseIcons();
		return;
	}

	let templateLists = compileTemplateLists(civCode);

	for (let u of templateLists.units.keys())
		if (!g_ParsedData.units[u])
			g_ParsedData.units[u] = loadUnit(u);

	for (let s of templateLists.structures.keys())
		if (!g_ParsedData.structures[s])
			g_ParsedData.structures[s] = loadStructure(s);

	// Load technologies
	g_ParsedData.techs[civCode] = {};
	for (let techcode of templateLists.techs.keys())
		if (basename(techcode).startsWith("phase"))
			g_ParsedData.phases[techcode] = loadPhase(techcode);
		else
			g_ParsedData.techs[civCode][techcode] = loadTechnology(techcode);

	// Establish phase order
	g_ParsedData.phaseList = UnravelPhases(g_ParsedData.phases);

	// Load any required generic phases that aren't already loaded
	for (let phasecode of g_ParsedData.phaseList)
		if (!g_ParsedData.phases[phasecode])
			g_ParsedData.phases[phasecode] = loadPhase(phasecode);

	// Group production and upgrade lists of structures by phase
	for (let structCode of templateLists.structures.keys())
	{
		let structInfo = g_ParsedData.structures[structCode];
		structInfo.phase = getPhaseOfTemplate(structInfo);
		let structPhaseIdx = g_ParsedData.phaseList.indexOf(structInfo.phase);

		// If this building is shared with another civ,
		// it may have already gone through the grouping process already
		if (!Array.isArray(structInfo.production.technology))
			continue;

		// Sort techs by phase
		let newProdTech = {};
		for (let prod of structInfo.production.technology)
		{
			let phase = getPhaseOfTechnology(prod);
			if (phase === false)
				continue;

			if (g_ParsedData.phaseList.indexOf(phase) < structPhaseIdx)
				phase = structInfo.phase;

			if (!(phase in newProdTech))
				newProdTech[phase] = [];

			newProdTech[phase].push(prod);
		}

		// Sort units by phase
		let newProdUnits = {};
		for (let prod of structInfo.production.units)
		{
			let phase = getPhaseOfTemplate(g_ParsedData.units[prod]);
			if (phase === false)
				continue;

			if (g_ParsedData.phaseList.indexOf(phase) < structPhaseIdx)
				phase = structInfo.phase;

			if (!(phase in newProdUnits))
				newProdUnits[phase] = [];

			newProdUnits[phase].push(prod);
		}

		g_ParsedData.structures[structCode].production = {
			"technology": newProdTech,
			"units": newProdUnits
		};

		// Sort upgrades by phase
		let newUpgrades = {};
		if (structInfo.upgrades)
			for (let upgrade of structInfo.upgrades)
			{
				let phase = getPhaseOfTemplate(upgrade);

				if (g_ParsedData.phaseList.indexOf(phase) < structPhaseIdx)
					phase = structInfo.phase;

				if (!newUpgrades[phase])
					newUpgrades[phase] = [];
				newUpgrades[phase].push(upgrade);
			}
		g_ParsedData.structures[structCode].upgrades = newUpgrades;
	}

	// Determine the buildList for the civ (grouped by phase)
	let buildList = {};
	let trainerList = [];
	for (let pha of g_ParsedData.phaseList)
		buildList[pha] = [];
	for (let structCode of templateLists.structures.keys())
	{
		let phase = g_ParsedData.structures[structCode].phase;
		buildList[phase].push(structCode);
	}

	for (let unitCode of templateLists.units.keys())
	{
		let unitTemplate = g_ParsedData.units[unitCode];
		if ((!unitTemplate.production || !Object.keys(unitTemplate.production).length) && !unitTemplate.upgrades)
			continue;

		trainerList.push(unitCode);
	}

	g_BuildList[g_SelectedCiv] = buildList;
	g_TrainList[g_SelectedCiv] = trainerList;

	draw();
	drawPhaseIcons();
}
