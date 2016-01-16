/**
 * Gets an array of all classes for this identity template
 */
function GetIdentityClasses(template)
{
	var classList = [];
	if (template.Classes && template.Classes._string)
		classList = classList.concat(template.Classes._string.split(/\s+/));

	if (template.VisibleClasses && template.VisibleClasses._string)
		classList = classList.concat(template.VisibleClasses._string.split(/\s+/));

	if (template.Rank)
		classList = classList.concat(template.Rank);
	return classList;
}

/**
 * Gets an array with all classes for this identity template
 * that should be shown in the GUI
 */
function GetVisibleIdentityClasses(template)
{
	if (template.VisibleClasses && template.VisibleClasses._string)
		return template.VisibleClasses._string.split(/\s+/);
	return [];
}

/**
 * Check if the classes given in the identity template
 * match a list of classes
 * @param classes List of the classes to check against
 * @param match Either a string in the form 
 *     "Class1 Class2+Class3"
 * where spaces are handled as OR and '+'-signs as AND,
 * and ! is handled as NOT, thus Class1+!Class2 = Class1 AND NOT Class2
 * Or a list in the form
 *     [["Class1"], ["Class2", "Class3"]]
 * where the outer list is combined as OR, and the inner lists are AND-ed
 * Or a hybrid format containing a list of strings, where the list is
 * combined as OR, and the strings are split by space and '+' and AND-ed
 *
 * @return undefined if there are no classes or no match object
 * true if the the logical combination in the match object matches the classes
 * false otherwise
 */
function MatchesClassList(classes, match)
{
	if (!match || !classes)
		return undefined;
	// transform the string to an array
	if (typeof match == "string")
		match = match.split(/\s+/);

	for (var sublist of match)
	{
		// if the elements are still strings, split them by space or by '+'
		if (typeof sublist == "string")
			sublist = sublist.split(/[+\s]+/);
		if (sublist.every(c => (c[0] == "!" && classes.indexOf(c.substr(1)) == -1)
		                    || (c[0] != "!" && classes.indexOf(c) != -1)))
			return true;
	}

	return false;
}

/**
 * Get information about a template with or without technology modifications.
 * @param template A valid template as returned by the template loader.
 * @param player An optional player id to get the technology modifications 
 *               of properties.
 */
function GetTemplateDataHelper(template, player)
{
	var ret = {};

	var func;
	if (player)
		func = ApplyValueModificationsToTemplate;
	else
		func = function(a, val, c, d) { return val; }

	if (template.Armour)
	{
		ret.armour = {
			"hack": func("Armour/Hack", +template.Armour.Hack, player, template),
			"pierce": func("Armour/Pierce", +template.Armour.Pierce, player, template),
			"crush": func("Armour/Crush", +template.Armour.Crush, player, template),
		};
	}

	if (template.Attack)
	{
		let getAttackStat = function(type, stat)
		{
			return func("Attack/"+type+"/"+stat, +(template.Attack[type][stat] || 0), player, template);
		};

		ret.attack = {};
		for (let type in template.Attack)
		{
			if (type == "Capture")
				ret.attack.Capture = {
					"value": getAttackStat(type,"Value"),
				};
			else
				ret.attack[type] = {
					"hack": getAttackStat(type, "Hack"),
					"pierce": getAttackStat(type, "Pierce"),
					"crush": getAttackStat(type, "Crush"),
					"minRange": getAttackStat(type, "MinRange"),
					"maxRange": getAttackStat(type, "MaxRange"),
					"elevationBonus": getAttackStat(type, "ElevationBonus"),
				};
			ret.attack[type].repeatTime = +(template.Attack[type].RepeatTime || 0);
		}
	}

	if (template.Auras)
	{
		ret.auras = {};
		for (let auraID in template.Auras)
		{
			let aura = template.Auras[auraID];
			if (aura.AuraName)
				ret.auras[auraID] = {
					"name": aura.AuraName,
					"description": aura.AuraDescription || null
				};
		}
	}

	if (template.BuildRestrictions)
	{
		// required properties
		ret.buildRestrictions = {
			"placementType": template.BuildRestrictions.PlacementType,
			"territory": template.BuildRestrictions.Territory,
			"category": template.BuildRestrictions.Category,
		};

		// optional properties
		if (template.BuildRestrictions.Distance)
		{
			ret.buildRestrictions.distance = {
				"fromCategory": template.BuildRestrictions.Distance.FromCategory,
			};
			if (template.BuildRestrictions.Distance.MinDistance) ret.buildRestrictions.distance.min = +template.BuildRestrictions.Distance.MinDistance;
			if (template.BuildRestrictions.Distance.MaxDistance) ret.buildRestrictions.distance.max = +template.BuildRestrictions.Distance.MaxDistance;
		}
	}

	if (template.TrainingRestrictions)
	{
		ret.trainingRestrictions = {
			"category": template.TrainingRestrictions.Category,
		};
	}

	if (template.Cost)
	{
		ret.cost = {};
		if (template.Cost.Resources.food) ret.cost.food = func("Cost/Resources/food", +template.Cost.Resources.food, player, template);
		if (template.Cost.Resources.wood) ret.cost.wood = func("Cost/Resources/wood", +template.Cost.Resources.wood, player, template);
		if (template.Cost.Resources.stone) ret.cost.stone = func("Cost/Resources/stone", +template.Cost.Resources.stone, player, template);
		if (template.Cost.Resources.metal) ret.cost.metal = func("Cost/Resources/metal", +template.Cost.Resources.metal, player, template);
		if (template.Cost.Population) ret.cost.population = func("Cost/Population", +template.Cost.Population, player, template);
		if (template.Cost.PopulationBonus) ret.cost.populationBonus = func("Cost/PopulationBonus", +template.Cost.PopulationBonus, player, template);
		if (template.Cost.BuildTime) ret.cost.time = func("Cost/BuildTime", +template.Cost.BuildTime, player, template);
	}

	if (template.Footprint)
	{
		ret.footprint = {"height": template.Footprint.Height};

		if (template.Footprint.Square)
			ret.footprint.square = {"width": +template.Footprint.Square["@width"], "depth": +template.Footprint.Square["@depth"]};
		else if (template.Footprint.Circle)
			ret.footprint.circle = {"radius": +template.Footprint.Circle["@radius"]};
		else
			warn("GetTemplateDataHelper(): Unrecognized Footprint type");
	}

	if (template.Obstruction)
	{
		ret.obstruction = {
			"active": ("" + template.Obstruction.Active == "true"),
			"blockMovement": ("" + template.Obstruction.BlockMovement == "true"),
			"blockPathfinding": ("" + template.Obstruction.BlockPathfinding == "true"),
			"blockFoundation": ("" + template.Obstruction.BlockFoundation == "true"),
			"blockConstruction": ("" + template.Obstruction.BlockConstruction == "true"),
			"disableBlockMovement": ("" + template.Obstruction.DisableBlockMovement == "true"),
			"disableBlockPathfinding": ("" + template.Obstruction.DisableBlockPathfinding == "true"),
			"shape": {}
		};

		if (template.Obstruction.Static)
		{
			ret.obstruction.shape.type = "static";
			ret.obstruction.shape.width = +template.Obstruction.Static["@width"];
			ret.obstruction.shape.depth = +template.Obstruction.Static["@depth"];
		}
		else if (template.Obstruction.Unit)
		{
			ret.obstruction.shape.type = "unit";
			ret.obstruction.shape.radius = +template.Obstruction.Unit["@radius"];
		}
		else
		{
			ret.obstruction.shape.type = "cluster";
		}
	}

	if (template.Pack)
	{
		ret.pack = {
			"state": template.Pack.State,
			"time": func("Pack/Time", +template.Pack.Time, player, template),
		};
	}

	if (template.Health)
		ret.health = Math.round(func("Health/Max", +template.Health.Max, player, template));

	if (template.Identity)
	{
		ret.selectionGroupName = template.Identity.SelectionGroupName;
		ret.name = {
			"specific": (template.Identity.SpecificName || template.Identity.GenericName),
			"generic": template.Identity.GenericName
		};
		ret.icon = template.Identity.Icon;
		ret.tooltip =  template.Identity.Tooltip;
		ret.gateConversionTooltip =  template.Identity.GateConversionTooltip;
		ret.requiredTechnology = template.Identity.RequiredTechnology;
		ret.visibleIdentityClasses = GetVisibleIdentityClasses(template.Identity);
	}

	if (template.UnitMotion)
	{
		ret.speed = {
			"walk": func("UnitMotion/WalkSpeed", +template.UnitMotion.WalkSpeed, player, template),
		};
		if (template.UnitMotion.Run)
			ret.speed.run = func("UnitMotion/Run/Speed", +template.UnitMotion.Run.Speed, player, template);
	}

	if (template.Trader)
	{
		ret.trader = {
			"GainMultiplier": func("Trader/GainMultiplier", +template.Trader.GainMultiplier, player, template)
		};
	}
	    
	if (template.WallSet)
	{
		ret.wallSet = {
			"templates": {
				"tower": template.WallSet.Templates.Tower,
				"gate": template.WallSet.Templates.Gate,
				"long": template.WallSet.Templates.WallLong,
				"medium": template.WallSet.Templates.WallMedium,
				"short": template.WallSet.Templates.WallShort,
			},
			"maxTowerOverlap": +template.WallSet.MaxTowerOverlap,
			"minTowerOverlap": +template.WallSet.MinTowerOverlap,
		};
	}

	if (template.WallPiece)
		ret.wallPiece = {"length": +template.WallPiece.Length};

	return ret;
}

/**
 * Get information about a technology template.
 * @param template A valid template as obtained by loading the tech JSON file.
 * @param civ Civilization for which the specific name should be returned.
 */
function GetTechnologyDataHelper(template, civ)
{
	var ret = {};

	// Get specific name for this civ or else the generic specific name 
	var specific = undefined;
	if (template.specificName)
	{
		if (template.specificName[civ])
			specific = template.specificName[civ];
		else
			specific = template.specificName['generic'];
	}

	ret.name = {
		"specific": specific,
		"generic": template.genericName,
	};

	if (template.icon)
		ret.icon = "technologies/" + template.icon;
	else
		ret.icon = null;
	ret.cost = {
		"food": template.cost ? (+template.cost.food) : 0,
		"wood": template.cost ? (+template.cost.wood) : 0,
		"metal": template.cost ? (+template.cost.metal) : 0,
		"stone": template.cost ? (+template.cost.stone) : 0,
		"time": template.researchTime ? (+template.researchTime) : 0,
	}
	ret.tooltip = template.tooltip;

	if (template.requirementsTooltip)
		ret.requirementsTooltip = template.requirementsTooltip;
	else
		ret.requirementsTooltip = "";

	if (template.requirements && template.requirements.class)
		ret.classRequirements = {"class": template.requirements.class, "number": template.requirements.number};

	ret.description = template.description;

	return ret;
}
