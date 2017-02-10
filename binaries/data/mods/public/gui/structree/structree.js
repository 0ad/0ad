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
	g_ParsedData.techs[civCode] = {};

	// get initial units
	for (let entity of g_CivData[civCode].StartEntities)
	{
		if (entity.Template.slice(0, 5) == "units")
			g_Lists.units.push(entity.Template);
		else if (entity.Template.slice(0, 6) == "struct")
			g_Lists.structures.push(entity.Template);
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
		let realcode = basename(techcode);

		if (realcode.slice(0,4) == "pair" || realcode.indexOf("_pair") > -1)
			techPairs[techcode] = loadTechnologyPair(techcode);
		else if (realcode.slice(0,5) == "phase")
			g_ParsedData.phases[techcode] = loadPhase(techcode);
		else
			g_ParsedData.techs[civCode][techcode] = loadTechnology(techcode);
	}

	// Expand tech pairs
	for (let paircode in techPairs)
	{
		let pair = techPairs[paircode];
		if (pair.reqs === false)
			continue;

		for (let techcode of pair.techs)
		{
			if (basename(techcode).slice(0, 5) === "phase")
				g_ParsedData.phases[techcode] = loadPhase(techcode);
			else
			{
				let newTech = loadTechnology(techcode);

				if (!newTech.reqs)
					newTech.reqs = {};
				else if (newTech.reqs === false)
					continue;

				for (let option of pair.reqs)
					for (let type in option)
						for (let opt in newTech.reqs)
						{
							if (!newTech.reqs[opt][type])
								newTech.reqs[opt][type] = [];
							newTech.reqs[opt][type] = newTech.reqs[opt][type].concat(option[type]);
						}

				g_ParsedData.techs[civCode][techcode] = newTech;
			}
		}
	}

	// Establish phase order
	g_ParsedData.phaseList = unravelPhases(g_ParsedData.techs[civCode]);
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
		structInfo.phase = GetPhaseOfTemplate(structInfo);
		let structPhaseIdx = g_ParsedData.phaseList.indexOf(structInfo.phase);

		// If this building is shared with another civ,
		// it may have already gone through the grouping process already
		if (!Array.isArray(structInfo.production.technology))
			continue;

		// Expand tech pairs
		for (let prod of structInfo.production.technology)
			if (prod in techPairs)
				structInfo.production.technology.splice(
					structInfo.production.technology.indexOf(prod),
					1, ...techPairs[prod].techs
				);

		// Sort techs by phase
		let newProdTech = {};
		for (let prod of structInfo.production.technology)
		{
			let phase = GetPhaseOfTechnology(prod);
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
			if (!g_ParsedData.units[prod])
				continue;

			let phase = GetPhaseOfTemplate(g_ParsedData.units[prod]);
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
	}

	// Determine the buildList for the civ (grouped by phase)
	let buildList = {};
	let trainerList = [];
	for (let pha of g_ParsedData.phaseList)
		buildList[pha] = [];
	for (let structCode of g_Lists.structures)
	{
		let phase = g_ParsedData.structures[structCode].phase;
		buildList[phase].push(structCode);
	}
	for (let unitCode of g_Lists.units)
		if (g_ParsedData.units[unitCode] && g_ParsedData.units[unitCode].production && Object.keys(g_ParsedData.units[unitCode].production).length)
		{
			// Replace any pair techs with the actual techs of that pair
			if (g_ParsedData.units[unitCode].production.techs)
				for (let prod of g_ParsedData.units[unitCode].production.techs)
					if (prod in techPairs)
						g_ParsedData.units[unitCode].production.techs.splice(
							g_ParsedData.units[unitCode].production.techs.indexOf(prod),
							1, ...techPairs[prod].techs
						);

			trainerList.push(unitCode);
		}

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
