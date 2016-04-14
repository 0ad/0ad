/**
 * Calculates gather rates.
 *
 * All available rates that have a value greater than 0 are summed and averaged
 */
function getGatherRates(templateName)
{
	// TODO: It would be nice to use the gather rates present in the templates
	// instead of hard-coding the possible rates here.

	// We ignore ruins here, as those are not that common and would skew the results
	var types = {
		"food": ["food", "food.fish", "food.fruit", "food.grain", "food.meat", "food.milk"],
		"wood": ["wood", "wood.tree"],
		"stone": ["stone", "stone.rock"],
		"metal": ["metal", "metal.ore"]
	};
	var rates = {};

	for (let type in types)
	{
		let count, rate;
		[rate, count] = types[type].reduce(function(sum, t) {
				let r = +fetchValue(templateName, "ResourceGatherer/Rates/"+t);
				return [sum[0] + (r > 0 ? r : 0), sum[1] + (r > 0 ? 1 : 0)];
			}, [0, 0]);

		if (rate > 0)
			rates[type] = Math.round(rate / count * 100) / 100;
	}

	if (!Object.keys(rates).length)
		return null;

	return rates;
}

function loadUnit(templateName)
{
	if (!Engine.TemplateExists(templateName))
		return null;
	var template = loadTemplate(templateName);

	var unit = GetTemplateDataHelper(template, null, g_AuraData);
	unit.phase = false;

	if (unit.requiredTechnology)
	{
		if (depath(unit.requiredTechnology).slice(0, 5) == "phase")
			unit.phase = unit.requiredTechnology;
		else if (unit.requiredTechnology.length)
			unit.required = unit.requiredTechnology;
	}

	unit.gather = getGatherRates(templateName);

	if (template.ProductionQueue)
	{
		unit.trainer = [];
		for (let build of template.ProductionQueue.Entities._string.split(" "))
		{
			build = build.replace("{civ}", g_SelectedCiv);
			unit.trainer.push(build);
			if (g_Lists.units.indexOf(build) === -1)
				g_Lists.units.push(build);
		}
	}

	if (template.Heal)
		unit.healer = {
			"Range": +template.Heal.Range || 0,
			"HP": +template.Heal.HP || 0,
			"Rate": +template.Heal.Rate || 0
		};

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
	var template = loadTemplate(templateName);
	var structure = GetTemplateDataHelper(template, null, g_AuraData);
	structure.phase = false;

	if (structure.requiredTechnology)
	{
		if (depath(structure.requiredTechnology).slice(0, 5) == "phase")
			structure.phase = structure.requiredTechnology;
		else if (structure.requiredTechnology.length)
			structure.required = structure.requiredTechnology;
	}

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

			if (health.min > wPart.health)
				health.min = wPart.health;
			else if (health.max < wPart.health)
				health.max = wPart.health;
		}
		if (health.min == health.max)
			structure.health = health.min;
		else
			structure.health = sprintf(translate("%(val1)s to %(val2)s"), {
				val1: health.min,
				val2: health.max
			});
	}

	return structure;
}

function loadTechnology(techName)
{
	var template = loadTechData(techName);
	var tech = GetTechnologyDataHelper(template, g_SelectedCiv);
	tech.reqs = {};

	if (template.pair !== undefined)
		tech.pair = template.pair;

	if (template.requirements !== undefined)
	{
		for (let op in template.requirements)
		{
			let val = template.requirements[op];	
			let req = calcReqs(op, val);

			switch (op)
			{
			case "tech":
				tech.reqs.generic = req;
				break;

			case "civ":
				tech.reqs[req] = [];
				break;

			case "any":
				if (req[0].length > 0)
					for (let r of req[0])
					{
						let v = req[0][r];
						if (typeof r == "number")
							tech.reqs[v] = [];
						else
							tech.reqs[r] = v;
					}
				if (req[1].length > 0)
					tech.reqs.generic = req[1];
				break;

			case "all":
				if (req[0].length < 1)
					tech.reqs.generic = req[1];
				else
					for (let r of req[0])
						tech.reqs[r] = req[1];
				break;
			}
		}
	}

	if (template.supersedes !== undefined)
	{
		if (tech.reqs.generic !== undefined)
			tech.reqs.generic.push(template.supersedes);
		else
			for (let ck of Object.keys(tech.reqs))
				tech.reqs[ck].push(template.supersedes);
	}

	return tech;
}

function loadPhase(phaseCode)
{
	var template = loadTechData(phaseCode);
	var phase = GetTechnologyDataHelper(template, g_SelectedCiv);

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
		"req": (pairInfo.supersedes !== undefined) ? pairInfo.supersedes : ""
	};
}

/**
 * Calculate the prerequisite requirements of a technology.
 * Works recursively if needed.
 *
 * @param op The base operation. Can be "civ", "tech", "all" or "any".
 * @param val The value associated with the above operation.
 *
 * @return Sorted requirments.
 */
function calcReqs(op, val)
{
	switch (op)
	{
	case "civ":
	case "class":
	case "notciv":
	case "number":
		// nothing needs doing
		break;

	case "tech":
		if (depath(val).slice(0,4) === "pair")
			return loadTechnologyPair(val).techs;
		return [ val ];

	case "all":
	case "any":
		let t = [];
		let c = [];
		for (let nv of val)
		{
			for (let o in nv)
			{
				let v = nv[o];
				let r = calcReqs(o, v);
				switch (o)
				{
				case "civ":
				case "notciv":
					c.push(r);
					break;

				case "tech":
					t = t.concat(r);
					break;

				case "any":
					c = c.concat(r[0]);
					t = t.concat(r[1]);
					break;

				case "all":
					for (let ci in r[0])
						c[ci] = r[1];
					t = t;
				}
			}
		}
		return [ c, t ];

	default:
		warn("Unknown reqs operator: "+op);
	}
	return val;
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

		if (!("generic" in techdata.reqs) || techdata.reqs.generic.length < 2)
			continue;

		let reqTech = techs[techcode].reqs.generic[1];

		// Tech that can't be researched anywhere
		if (!(reqTech in techs))
			continue;

		if (!("generic" in techs[reqTech].reqs))
			continue;

		let reqPhase = techs[reqTech].reqs.generic[0];
		let myPhase = techs[techcode].reqs.generic[0];

		if (reqPhase == myPhase || depath(reqPhase).slice(0,5) !== "phase" || depath(myPhase).slice(0,5) !== "phase")
			continue;

		let reqPhasePos = phaseList.indexOf(reqPhase);
		let myPhasePos = phaseList.indexOf(myPhase);

		if (phaseList.length === 0)
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
