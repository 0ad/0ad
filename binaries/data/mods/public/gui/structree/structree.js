var g_ParsedData = {
	"units": {},
	"structures": {},
	"techs": {},
	"phases": {}
};

var g_Lists = {};
var g_CivData = {};
var g_SelectedCiv = "";
var g_CallbackSet = false;
var g_ResourceData = new Resources();

/**
 * Initialize the dropdown containing all the available civs
 */
function init(data = {})
{
	g_CivData = loadCivData(true);

	let civList = Object.keys(g_CivData).map(civ => ({
		"name": g_CivData[civ].Name,
		"code": civ,
	})).sort(sortNameIgnoreCase);

	if (!civList.length)
		return;

	var civSelection = Engine.GetGUIObjectByName("civSelection");
	civSelection.list = civList.map(c => c.name);
	civSelection.list_data = civList.map(c => c.code);

	if(data.civ)
	{
		civSelection.selected = civSelection.list_data.indexOf(data.civ);
		selectCiv(data.civ);
	}
	else
		civSelection.selected = 0;

	if (data.callback)
		g_CallbackSet = true;
}

function selectCiv(civCode)
{
	if (civCode === g_SelectedCiv || !g_CivData[civCode])
		return;

	g_SelectedCiv = civCode;

	// If a buildList already exists, then this civ has already been parsed
	if (g_CivData[g_SelectedCiv].buildList)
	{
		draw();
		return;
	}

	g_Lists = {
		"units": [],
		"structures": [],
		"techs": []
	};

	// get initial units
	var startStructs = [];
	for (let entity of g_CivData[civCode].StartEntities)
	{
		if (entity.Template.slice(0, 5) == "units")
			g_Lists.units.push(entity.Template);
		else if (entity.Template.slice(0, 6) == "struct")
		{
			g_Lists.structures.push(entity.Template);
			startStructs.push(entity.Template);
		}
	}

	// Load units and structures
	var unitCount = 0;
	do
	{
		for (let u of g_Lists.units)
			if (!g_ParsedData.units[u])
				g_ParsedData.units[u] = loadUnit(u);

		unitCount = g_Lists.units.length;

		for (let s of g_Lists.structures)
			if (!g_ParsedData.structures[s])
				g_ParsedData.structures[s] = loadStructure(s);

	} while (unitCount < g_Lists.units.length);

	// Load technologies
	var techPairs = {};
	for (let techcode of g_Lists.techs)
	{
		let realcode = depath(techcode);

		if (realcode.slice(0,4) == "pair" || realcode.indexOf("_pair") > -1)
			techPairs[techcode] = loadTechnologyPair(techcode);
		else if (realcode.slice(0,5) == "phase")
			g_ParsedData.phases[techcode] = loadPhase(techcode);
		else
			g_ParsedData.techs[techcode] = loadTechnology(techcode);
	}

	// Expand tech pairs
	for (let paircode in techPairs)
	{
		let pair = techPairs[paircode];
		for (let techcode of pair.techs)
		{
			if (depath(techcode).slice(0, 5) === "phase")
				g_ParsedData.phases[techcode] = loadPhase(techcode);
			else
			{
				let newTech = loadTechnology(techcode);
				if (pair.req !== "")
				{
					if ("generic" in newTech.reqs)
						newTech.reqs.generic.concat(techPairs[pair.req].techs);
					else
					{
						for (let civkey of Object.keys(newTech.reqs))
							newTech.reqs[civkey].concat(techPairs[pair.req].techs);
					}
				}
				g_ParsedData.techs[techcode] = newTech;
			}
		}
	}

	// Establish phase order
	g_ParsedData.phaseList = unravelPhases(g_ParsedData.techs);
	for (let phasecode of g_ParsedData.phaseList)
	{
		let phaseInfo = loadTechData(phasecode);
		g_ParsedData.phases[phasecode] = loadPhase(phasecode);

		if (!("requirements" in phaseInfo))
			continue;

		for (let op in phaseInfo.requirements)
		{
			let val = phaseInfo.requirements[op];
			if (op != "any")
				continue;

			for (let v of val)
			{
				let k = Object.keys(v);
				k = k[0];
				v = v[k];
				if (k != "tech")
					continue;

				if (v in g_ParsedData.phases)
					g_ParsedData.phases[v].actualPhase = phasecode;
				else if (v in techPairs)
				{
					for (let t of techPairs[v].techs)
						g_ParsedData.phases[t].actualPhase = phasecode;
				}
			}
		}
	}

	// Group production lists of structures by phase
	for (let structCode of g_Lists.structures)
	{
		let structInfo = g_ParsedData.structures[structCode];
		let structPhaseIdx = g_ParsedData.phaseList.indexOf(structInfo.phase);

		// If this building is shared with another civ,
		// it may have already gone through the grouping process already
		if (!Array.isArray(structInfo.production.technology))
			continue;

		// Expand tech pairs
		for (let prod of structInfo.production.technology)
			if (prod in techPairs)
				structInfo.production.technology.splice(
					structInfo.production.technology.indexOf(prod), 1,
					techPairs[prod].techs[0], techPairs[prod].techs[1]
				);

		// Sort techs by phase
		let newProdTech = {};
		for (let prod of structInfo.production.technology)
		{
			let phase = "";

			if (depath(prod).slice(0,5) === "phase")
			{
				phase = g_ParsedData.phaseList.indexOf(g_ParsedData.phases[prod].actualPhase);
				if (phase > 0)
					phase = g_ParsedData.phaseList[phase - 1];
			}
			else if (g_SelectedCiv in g_ParsedData.techs[prod].reqs)
			{
				for (let req of g_ParsedData.techs[prod].reqs[g_SelectedCiv])
					if (depath(req).slice(0,5) === "phase")
						phase = req;
			}
			else if ("generic" in g_ParsedData.techs[prod].reqs)
			{
				for (let req of g_ParsedData.techs[prod].reqs.generic)
					if (depath(req).slice(0,5) === "phase")
						phase = req;
			}

			if (depath(phase).slice(0,5) !== "phase" ||
			    g_ParsedData.phaseList.indexOf(phase) < structPhaseIdx)
			{
				if (structInfo.phase !== false)
					phase = structInfo.phase;
				else
					phase = g_ParsedData.phaseList[0];
			}

			if (!(phase in newProdTech))
				newProdTech[phase] = [];

			newProdTech[phase].push(prod);
		}

		// Determine phase for units
		let newProdUnits = {};
		for (let prod of structInfo.production.units)
		{
			if (!g_ParsedData.units[prod])
				continue;

			let unit = g_ParsedData.units[prod];
			let phase = "";

			if (unit.phase !== false)
			{
				if (g_ParsedData.phaseList.indexOf(unit.phase) < 0)
					phase = g_ParsedData.phases[unit.phase].actualPhase;
				else
					phase = unit.phase;
			}
			else if (unit.required !== undefined)
			{
				if (g_ParsedData.phases[unit.required])
					phase = g_ParsedData.phases[unit.required].actualPhase;
				else if (g_ParsedData.techs[unit.required])
				{
					let reqs = g_ParsedData.techs[unit.required].reqs;
					if (reqs[g_SelectedCiv])
						phase = reqs[g_SelectedCiv][0];
					else if (reqs.generic)
						phase = reqs.generic[0];
					else
						warn("Empty requirements found on technology " + unit.required);
				}
				else
					warn("Technology " + unit.required + " for " + prod + " not found.");
			}

			if (depath(phase).slice(0,5) !== "phase" || g_ParsedData.phaseList.indexOf(phase) < structPhaseIdx)
				if (structInfo.phase !== false)
					phase = structInfo.phase;
				else
					phase = g_ParsedData.phaseList[0];

			if (!(phase in newProdUnits))
				newProdUnits[phase] = [];

			newProdUnits[phase].push(prod);
		}

		g_ParsedData.structures[structCode].production = {
			"technology": newProdTech,
			"units": newProdUnits
		};
	}

	// Determine the buildList for the civ (grouped by phase)
	var buildList = {};
	var trainerList = [];
	for (let pha of g_ParsedData.phaseList)
		buildList[pha] = [];
	for (let structCode of g_Lists.structures)
	{
		if (!g_ParsedData.structures[structCode].phase || startStructs.indexOf(structCode) > -1)
			g_ParsedData.structures[structCode].phase = g_ParsedData.phaseList[0];

		let myPhase = g_ParsedData.structures[structCode].phase;
		if (g_ParsedData.phaseList.indexOf(myPhase) === -1)
			myPhase = g_ParsedData.phases[myPhase].actualPhase;

		buildList[myPhase].push(structCode);
	}
	for (let unitCode of g_Lists.units)
		if (g_ParsedData.units[unitCode] && g_ParsedData.units[unitCode].production)
			trainerList.push(unitCode);

	g_CivData[g_SelectedCiv].buildList = buildList;
	g_CivData[g_SelectedCiv].trainList = trainerList;

	draw();
}

function closeStrucTree()
{
	if (g_CallbackSet)
		Engine.PopGuiPageCB(0);
	else
		Engine.PopGuiPage();
}
