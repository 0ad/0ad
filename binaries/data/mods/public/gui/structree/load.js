function loadUnit(templateName)
{
	if (!Engine.TemplateExists(templateName))
		return null;

	let template = loadTemplate(templateName);
	let unit = GetTemplateDataHelper(template, null, g_AuraData, g_ResourceData);

	if (template.ProductionQueue)
	{
		unit.production = {};
		if (template.ProductionQueue.Entities)
		{
			unit.production.units = [];
			for (let build of template.ProductionQueue.Entities._string.split(" "))
			{
				build = build.replace("{civ}", g_SelectedCiv);
				unit.production.units.push(build);
				if (g_Lists.units.indexOf(build) < 0)
					g_Lists.units.push(build);
			}
		}
		if (template.ProductionQueue.Technologies)
		{
			unit.production.techs = [];
			for (let research of template.ProductionQueue.Technologies._string.split(" "))
			{
				unit.production.techs.push(research);
				if (g_Lists.techs.indexOf(research) < 0)
					g_Lists.techs.push(research);
			}
		}
	}

	if (template.Builder && template.Builder.Entities._string)
		for (let build of template.Builder.Entities._string.split(" "))
		{
			build = build.replace("{civ}", g_SelectedCiv);
			if (g_Lists.structures.indexOf(build) < 0)
				g_Lists.structures.push(build);
		}

	return unit;
}

function loadStructure(templateName)
{
	let template = loadTemplate(templateName);
	let structure = GetTemplateDataHelper(template, null, g_AuraData, g_ResourceData);

	structure.production = {
		"technology": [],
		"units": []
	};

	if (template.ProductionQueue)
	{
		if (template.ProductionQueue.Entities && template.ProductionQueue.Entities._string)
			for (let build of template.ProductionQueue.Entities._string.split(" "))
			{
				build = build.replace("{civ}", g_SelectedCiv);
				structure.production.units.push(build);
				if (g_Lists.units.indexOf(build) < 0)
					g_Lists.units.push(build);
			}

		if (template.ProductionQueue.Technologies && template.ProductionQueue.Technologies._string)
			for (let research of template.ProductionQueue.Technologies._string.split(" "))
			{
				structure.production.technology.push(research);
				if (g_Lists.techs.indexOf(research) < 0)
					g_Lists.techs.push(research);
			}
	}

	if (structure.wallSet)
	{
		structure.wallset = {};

		// Note: Assume wall segments of all lengths have the same armor and auras
		let struct = loadStructure(structure.wallSet.templates.long);
		structure.armour = struct.armour;
		structure.auras = struct.auras;

		// For technology cost multiplier, we need to use the tower
		struct = loadStructure(structure.wallSet.templates.tower);
		structure.techCostMultiplier = struct.techCostMultiplier;

		let health;

		for (let wSegm in structure.wallSet.templates)
		{
			let wPart = loadStructure(structure.wallSet.templates[wSegm]);
			structure.wallset[wSegm] = wPart;

			for (let research of wPart.production.technology)
				structure.production.technology.push(research);

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
		if (health.min == health.max)
			structure.health = health.min;
		else
			structure.health = sprintf(translate("%(val1)s to %(val2)s"), {
				"val1": health.min,
				"val2": health.max
			});
	}

	return structure;
}

function loadTechnology(techName)
{
	let template = loadTechData(techName);
	let tech = GetTechnologyDataHelper(template, g_SelectedCiv, g_ResourceData);

	if (template.pair !== undefined)
		tech.pair = template.pair;

	return tech;
}

function loadPhase(phaseCode)
{
	var template = loadTechData(phaseCode);
	var phase = GetTechnologyDataHelper(template, g_SelectedCiv, g_ResourceData);

	phase.actualPhase = phaseCode;
	if (template.replaces !== undefined)
		phase.actualPhase = template.replaces[0];

	return phase;
}

function loadTechnologyPair(pairCode)
{
	var pairInfo = loadTechData(pairCode);

	return {
		"techs": [ pairInfo.top, pairInfo.bottom ],
		"reqs": DeriveTechnologyRequirements(pairInfo, g_SelectedCiv)
	};
}

/**
 * Unravel phases
 *
 * @param techs The current available store of techs
 *
 * @return List of phases
 */
function unravelPhases(techs)
{
	var phaseList = [];

	for (let techcode in techs)
	{
		let techdata = techs[techcode];

		if (!techdata.reqs || !techdata.reqs.length || !techdata.reqs[0].techs || techdata.reqs[0].techs.length < 2)
			continue;

		let reqTech = techs[techcode].reqs[0].techs[1];

		if (!techs[reqTech] || !techs[reqTech].reqs.length)
			continue;

		let reqPhase = techs[reqTech].reqs[0].techs[0];
		let myPhase = techs[techcode].reqs[0].techs[0];

		if (reqPhase == myPhase || basename(reqPhase).slice(0,5) !== "phase" || basename(myPhase).slice(0,5) !== "phase")
			continue;

		let reqPhasePos = phaseList.indexOf(reqPhase);
		let myPhasePos = phaseList.indexOf(myPhase);

		if (!phaseList.length)
			phaseList = [reqPhase, myPhase];
		else if (reqPhasePos < 0 && myPhasePos > -1)
			phaseList.splice(myPhasePos, 0, reqPhase);
		else if (myPhasePos < 0 && reqPhasePos > -1)
			phaseList.splice(reqPhasePos+1, 0, myPhase);
		else if (reqPhasePos > myPhasePos)
		{
			phaseList.splice(reqPhasePos+1, 0, myPhase);
			phaseList.splice(myPhasePos, 1);
		}
	}
	return phaseList;
}
