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
 *
 * NOTICE: The data returned here should have the same structure as
 * the object returned by GetEntityState and GetExtendedEntityState!
 *
 * @param template A valid template as returned by the template loader.
 * @param player An optional player id to get the technology modifications
 *               of properties.
 * @param auraTemplates An object in the form of {key: {auraName: "", auraDescription: ""}}
 */
function GetTemplateDataHelper(template, player, auraTemplates)
{
	// Return data either from template (in tech tree) or sim state (ingame)
	let getEntityValue = function(tech_type) {

		let current_value = template;
		for (let property of tech_type.split("/"))
			current_value = current_value[property] || 0;
		current_value = +current_value;

		if (!player)
			return current_value;

		return ApplyValueModificationsToTemplate(tech_type, current_value, player, template);
	};

	let ret = {};

	if (template.Armour)
		ret.armour = {
			"hack": getEntityValue("Armour/Hack"),
			"pierce": getEntityValue("Armour/Pierce"),
			"crush": getEntityValue("Armour/Crush")
		};

	if (template.Attack)
	{
		ret.attack = {};
		for (let type in template.Attack)
		{
			let getAttackStat = function(stat) {
				return getEntityValue("Attack/" + type + "/" + stat);
			};

			if (type == "Capture")
				ret.attack.Capture = {
					"value": getAttackStat("Value")
				};
			else
				ret.attack[type] = {
					"hack": getAttackStat("Hack"),
					"pierce": getAttackStat("Pierce"),
					"crush": getAttackStat("Crush"),
					"minRange": getAttackStat("MinRange"),
					"maxRange": getAttackStat("MaxRange"),
					"elevationBonus": getAttackStat("ElevationBonus")
				};

			ret.attack[type].repeatTime = getAttackStat("RepeatTime");

			if (template.Attack[type].Splash)
				ret.attack[type].splash = {
					"hack": getAttackStat("Splash/Hack"),
					"pierce": getAttackStat("Splash/Pierce"),
					"crush": getAttackStat("Splash/Crush"),
					// true if undefined
					"friendlyFire": template.Attack[type].Splash.FriendlyFire != "false"
				};
		}
	}

	if (template.Auras)
	{
		ret.auras = {};
		for (let auraID of template.Auras._string.split(/\s+/))
		{
			let aura = auraTemplates[auraID];
			if (aura.auraName)
				ret.auras[auraID] = {
					"name": aura.auraName,
					"description": aura.auraDescription || null
				};
		}
	}

	if (template.BuildingAI)
		ret.buildingAI = {
			"defaultArrowCount": getEntityValue("BuildingAI/DefaultArrowCount"),
			"garrisonArrowMultiplier": getEntityValue("BuildingAI/GarrisonArrowMultiplier"),
			"maxArrowCount": getEntityValue("BuildingAI/MaxArrowCount")
		};

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
				"fromClass": template.BuildRestrictions.Distance.FromClass,
			};

			if (template.BuildRestrictions.Distance.MinDistance)
				ret.buildRestrictions.distance.min = +template.BuildRestrictions.Distance.MinDistance;

			if (template.BuildRestrictions.Distance.MaxDistance)
				ret.buildRestrictions.distance.max = +template.BuildRestrictions.Distance.MaxDistance;
		}
	}

	if (template.TrainingRestrictions)
		ret.trainingRestrictions = {
			"category": template.TrainingRestrictions.Category,
		};

	if (template.Cost)
	{
		ret.cost = {};
		if (template.Cost.Resources.food)
			ret.cost.food = getEntityValue("Cost/Resources/food");

		if (template.Cost.Resources.wood)
			ret.cost.wood = getEntityValue("Cost/Resources/wood");

		if (template.Cost.Resources.stone)
			ret.cost.stone = getEntityValue("Cost/Resources/stone");

		if (template.Cost.Resources.metal)
			ret.cost.metal = getEntityValue("Cost/Resources/metal");

		if (template.Cost.Population)
			ret.cost.population = getEntityValue("Cost/Population");

		if (template.Cost.PopulationBonus)
			ret.cost.populationBonus = getEntityValue("Cost/PopulationBonus");

		if (template.Cost.BuildTime)
			ret.cost.time = getEntityValue("Cost/BuildTime");
	}

	if (template.Footprint)
	{
		ret.footprint = { "height": template.Footprint.Height };

		if (template.Footprint.Square)
			ret.footprint.square = {
				"width": +template.Footprint.Square["@width"],
				"depth": +template.Footprint.Square["@depth"]
			};
		else if (template.Footprint.Circle)
			ret.footprint.circle = { "radius": +template.Footprint.Circle["@radius"] };
		else
			warn("GetTemplateDataHelper(): Unrecognized Footprint type");
	}

	if (template.GarrisonHolder)
	{
		ret.garrisonHolder = {
			"buffHeal": getEntityValue("GarrisonHolder/BuffHeal")
		};

		if (template.GarrisonHolder.Max)
			ret.garrisonHolder.capacity = getEntityValue("GarrisonHolder/Max");
	}

	if (template.Heal)
		ret.heal = {
			"hp": getEntityValue("Heal/HP"),
			"range": getEntityValue("Heal/Range"),
			"rate": getEntityValue("Heal/Rate")
		};

	if (template.Loot)
	{
		ret.loot = {};
		for (let type in template.Loot)
			ret.loot[type] = getEntityValue("Loot/"+ type);
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
			ret.obstruction.shape.type = "cluster";
	}

	if (template.Pack)
		ret.pack = {
			"state": template.Pack.State,
			"time": getEntityValue("Pack/Time"),
		};

	if (template.Health)
		ret.health = Math.round(getEntityValue("Health/Max"));

	if (template.Identity)
	{
		ret.selectionGroupName = template.Identity.SelectionGroupName;
		ret.name = {
			"specific": (template.Identity.SpecificName || template.Identity.GenericName),
			"generic": template.Identity.GenericName
		};
		ret.icon = template.Identity.Icon;
		ret.tooltip =  template.Identity.Tooltip;
		ret.requiredTechnology = template.Identity.RequiredTechnology;
		ret.visibleIdentityClasses = GetVisibleIdentityClasses(template.Identity);
	}

	if (template.UnitMotion)
	{
		ret.speed = {
			"walk": getEntityValue("UnitMotion/WalkSpeed"),
		};
		if (template.UnitMotion.Run)
			ret.speed.run = getEntityValue("UnitMotion/Run/Speed");
	}

	if (template.ProductionQueue)
	{
		ret.techCostMultiplier = {};
		for (let res in template.ProductionQueue.TechCostMultiplier)
			ret.techCostMultiplier[res] = getEntityValue("ProductionQueue/TechCostMultiplier/" + res);
	}

	if (template.Trader)
		ret.trader = {
			"GainMultiplier": getEntityValue("Trader/GainMultiplier")
		};

	if (template.WallSet)
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

	if (template.WallPiece)
		ret.wallPiece = { "length": +template.WallPiece.Length };

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
	var specific;
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

	ret.icon = template.icon ? "technologies/" + template.icon : null;

	ret.cost = {
		"food": template.cost ? +template.cost.food : 0,
		"wood": template.cost ? +template.cost.wood : 0,
		"metal": template.cost ? +template.cost.metal : 0,
		"stone": template.cost ? +template.cost.stone : 0,
		"time": template.researchTime ? +template.researchTime : 0,
	}

	ret.tooltip = template.tooltip;
	ret.requirementsTooltip = template.requirementsTooltip || "";

	if (template.requirements && template.requirements.class)
		ret.classRequirements = {
			"class": template.requirements.class,
			"number": template.requirements.number
		};

	ret.description = template.description;

	return ret;
}

function calculateCarriedResources(carriedResources, tradingGoods)
{
	var resources = {};

	if (carriedResources)
		for (let resource of carriedResources)
			resources[resource.type] = (resources[resource.type] || 0) + resource.amount;

	if (tradingGoods && tradingGoods.amount)
		resources[tradingGoods.type] =
			(resources[tradingGoods.type] || 0) +
			(tradingGoods.amount.traderGain || 0) +
			(tradingGoods.amount.market1Gain || 0) +
			(tradingGoods.amount.market2Gain || 0);

	return resources;
}

