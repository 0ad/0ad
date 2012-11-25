const GEOLOGY = "geology";
const FLORA = "flora";
const FAUNA = "fauna";
const SPECIAL = "special";

const COST_DISPLAY_NAMES = {
    "food": "[icon=\"iconFood\"]",
    "wood": "[icon=\"iconWood\"]",
    "stone": "[icon=\"iconStone\"]",
    "metal": "[icon=\"iconMetal\"]",
    "population": "[icon=\"iconPopulation\"]",
    "time": "[icon=\"iconTime\"]"
};

//-------------------------------- -------------------------------- --------------------------------
// Utility functions
//-------------------------------- -------------------------------- --------------------------------

function toTitleCase(word)
{
	if (word.length > 0)
	{
		var titleCased = word.substring(0, 1).toUpperCase();

		if (word.length > 1)
		{
			titleCased += word.substring(1).toLowerCase();
		}
		return titleCased;
	}

	return word;
}

// Get the basic player data
function getPlayerData(playerAssignments)
{
	var players = [];

	var simState = Engine.GuiInterfaceCall("GetSimulationState");
	if (!simState)
		return players;

	for (var i = 0; i < simState.players.length; i++)
	{
		var playerState = simState.players[i];

		var name = playerState.name;
		var civ = playerState.civ;
		var color = {
		    "r": playerState.colour.r*255, 
		    "g": playerState.colour.g*255, 
		    "b": playerState.colour.b*255, 
		    "a": playerState.colour.a*255
		};

		var player = {
		    "name": name,
		    "civ": civ,
		    "color": color,
		    "team": playerState.team,
		    "teamsLocked": playerState.teamsLocked,
		    "state": playerState.state,
		    "isAlly": playerState.isAlly,
		    "isNeutral": playerState.isNeutral,
		    "isEnemy": playerState.isEnemy,
		    "guid": undefined, // network guid for players controlled by hosts
		    "disconnected": false // flag for host-controlled players who have left the game
		};
		players.push(player);
	}

	// Overwrite default player names with multiplayer names
	if (playerAssignments)
	{
		for (var playerGuid in playerAssignments)
		{
			var playerAssignment = playerAssignments[playerGuid];
			if (players[playerAssignment.player])
			{
				players[playerAssignment.player].guid = playerGuid;
				players[playerAssignment.player].name = playerAssignment.name;
			}
		}
	}

	return players;
}

// Returns whether a player has physical allies.
function hasAllies(playerID, playerData)
{
	if (playerData[playerID] && playerData[playerID].team != -1)
	{
		for (var i = 0; i < playerData.length; i++)
			if (playerData[i].team == playerData[playerID].team)
				return true;
	}
	return false;
}

function findGuidForPlayerID(playerAssignments, player)
{
	for (var playerGuid in playerAssignments)
	{
		var playerAssignment = playerAssignments[playerGuid];
		if (playerAssignment.player == player)
			return playerGuid;
	}
	return undefined;
}

// Update player data when a host has connected
function updatePlayerDataAdd(players, hostGuid, playerAssignment)
{
	if (players[playerAssignment.player])
	{
		players[playerAssignment.player].guid = hostGuid;
		players[playerAssignment.player].name = playerAssignment.name;
		players[playerAssignment.player].offline = false;
	}
}

// Update player data when a host has disconnected
function updatePlayerDataRemove(players, hostGuid)
{
	for each (var player in players)
		if (player.guid == hostGuid)
			player.offline = true;
}

function hasClass(entState, className)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
			return (classes.indexOf(className) != -1);
	}
	return false;
}

// For the unit details panel
function damageValues(dmg)
{
	if (dmg)
	{
		var dmgArray = [];
		dmg.hack? dmgArray.push(dmg.hack) : dmgArray.push(0);
		dmg.pierce? dmgArray.push(dmg.pierce) : dmgArray.push(0);
		dmg.crush? dmgArray.push(dmg.crush) : dmgArray.push(0);

		return dmgArray;
	}
	else
	{
		return [0, 0, 0];
	}
}

// For the unit details panel
function damageTypeDetails(dmg)
{
	if (dmg)
	{
		var dmgArray = [];
		if (dmg.hack) dmgArray.push(dmg.hack + "[font=\"sans-10\"][color=\"orange\"] Hack[/color][/font]");
		if (dmg.pierce) dmgArray.push(dmg.pierce + "[font=\"sans-10\"][color=\"orange\"] Pierce[/color][/font]");
		if (dmg.crush) dmgArray.push(dmg.crush + "[font=\"sans-10\"][color=\"orange\"] Crush[/color][/font]");

		return dmgArray.join(", ");
	}
	else
	{
		return "[font=\"serif-12\"](None)[/font]";
	}
}

// For the training tooltip
function damageTypesToText(dmg)
{
	if (!dmg)
		return "[font=\"serif-12\"](None)[/font]";

	var hackLabel = "[font=\"serif-12\"] Hack[/font]";
	var pierceLabel = "[font=\"serif-12\"] Pierce[/font]";
	var crushLabel = "[font=\"serif-12\"] Crush[/font]";
	var hackDamage = dmg.hack;
	var pierceDamage = dmg.pierce;
	var crushDamage = dmg.crush;

	var dmgArray = [];
	if (hackDamage) dmgArray.push(hackDamage + hackLabel);
	if (pierceDamage) dmgArray.push(pierceDamage + pierceLabel);
	if (crushDamage) dmgArray.push(crushDamage + crushLabel);

	return dmgArray.join("[font=\"serif-12\"], [/font]");
}

function getEntityCommandsList(entState)
{
	var commands = [];
	if (entState.garrisonHolder)
	{
		commands.push({
		    "name": "unload-all",
		    "tooltip": "Unload All",
		    "icon": "garrison-out.png"
		});
	}

	commands.push({
	    "name": "delete",
	    "tooltip": "Delete",
	    "icon": "kill_small.png"
	});

	if (hasClass(entState, "Unit"))
	{
		commands.push({
		    "name": "stop",
		    "tooltip": "Stop",
		    "icon": "stop.png"
		});
		commands.push({
		    "name": "garrison",
		    "tooltip": "Garrison",
		    "icon": "garrison.png"
		});
	}

	if (entState.buildEntities)
	{
		commands.push({
		    "name": "repair",
		    "tooltip": "Repair",
		    "icon": "repair.png"
		});
	}

	if (entState.rallyPoint)
	{
		commands.push({
		    "name": "focus-rally",
		    "tooltip": "Focus on Rally Point",
		    "icon": "focus-rally.png"
		});
	}

	return commands;
}

/**
 * Translates a cost component identifier as they are used internally (e.g. "population", "food", etc.) to proper
 * display names.
 */
function getCostComponentDisplayName(costComponentName)
{
	return COST_DISPLAY_NAMES[costComponentName];
}

/**
 * Helper function for getEntityCostTooltip.
 */
function getEntityCostComponentsTooltipString(template, trainNum, entity)
{
	var totalCosts = {};
	if (!trainNum)
		trainNum = 1;
	for (var r in template.cost)
		totalCosts[r] = Math.floor(template.cost[r] * trainNum);
	totalCosts.time = entity ? Math.ceil(Engine.GuiInterfaceCall("GetBatchTime", {"entity": entity, "batchSize": trainNum})) : template.cost.time;

	var costs = [];
	if (totalCosts.food) costs.push(getCostComponentDisplayName("food") + " " + totalCosts.food);
	if (totalCosts.wood) costs.push(getCostComponentDisplayName("wood") + " " + totalCosts.wood);
	if (totalCosts.metal) costs.push(getCostComponentDisplayName("metal") + " " + totalCosts.metal);
	if (totalCosts.stone) costs.push(getCostComponentDisplayName("stone") + " " + totalCosts.stone);
	if (totalCosts.population) costs.push(getCostComponentDisplayName("population") + " " + totalCosts.population);
	if (totalCosts.time) costs.push(getCostComponentDisplayName("time") + " " + totalCosts.time);
	return costs;
}

/**
 * Returns an array of strings for a set of wall pieces. If the pieces share
 * resource type requirements, output will be of the form '10 to 30 Stone',
 * otherwise output will be, e.g. '10 Stone, 20 Stone, 30 Stone'.
 */
function getWallPieceTooltip(wallTypes)
{
	var out = [];
	var resourceCount = {};

	// Initialize the acceptable types for '$x to $y $resource' mode.
	for (var resource in wallTypes[0].cost)
		if (wallTypes[0].cost[resource])
			resourceCount[resource] = [wallTypes[0].cost[resource]];

	var sameTypes = true;
	for (var i = 1; i < wallTypes.length; ++i)
	{
		for (var resource in wallTypes[i].cost)
		{
			// Break out of the same-type mode if this wall requires
			// resource types that the first didn't.
			if (wallTypes[i].cost[resource] && !resourceCount[resource])
			{
				sameTypes = false;
				break;
			}
		}

		for (var resource in resourceCount)
		{
			if (wallTypes[i].cost[resource])
				resourceCount[resource].push(wallTypes[i].cost[resource]);
			else
			{
				sameTypes = false;
				break;
			}
		}
	}

	if (sameTypes)
	{
		for (var resource in resourceCount)
		{
			var resourceMin = Math.min.apply(Math, resourceCount[resource]);
			var resourceMax = Math.max.apply(Math, resourceCount[resource]);

			out.push(getCostComponentDisplayName(resource) + resourceMin + " to " + getCostComponentDisplayName(resource) + resourceMax);
		}
	}
	else
		for (var i = 0; i < wallTypes.length; ++i)
			out.push(getEntityCostComponentsTooltipString(wallTypes[i]).join(", "));

	return out;
}

/**
 * Returns the cost information to display in the specified entity's construction button tooltip.
 */
function getEntityCostTooltip(template, trainNum, entity)
{
	var cost = "[font=\"serif-bold-13\"]Costs:[/font] ";

	// Entities with a wallset component are proxies for initiating wall placement and as such do not have a cost of
	// their own; the individual wall pieces within it do.
	if (template.wallSet)
	{
		var templateLong = GetTemplateData(template.wallSet.templates.long);
		var templateMedium = GetTemplateData(template.wallSet.templates.medium);
		var templateShort = GetTemplateData(template.wallSet.templates.short);
		var templateTower = GetTemplateData(template.wallSet.templates.tower);

		var wallCosts = getWallPieceTooltip([templateShort, templateMedium, templateLong]);
		var towerCosts = getEntityCostComponentsTooltipString(templateTower);

		cost += "\n";
		cost += " Walls:  " + wallCosts.join("  ") + "\n";
		cost += " Towers: " + towerCosts.join("  ");
	}
	else if (template.cost)
	{
		var costs = getEntityCostComponentsTooltipString(template, trainNum, entity);
		cost += costs.join("  ");
	}
	else
	{
		cost = ""; // cleaner than duplicating the serif-bold-13 stuff
	}

	return cost;
}

/**
 * Returns the population bonus information to display in the specified entity's construction button tooltip.
 */
function getPopulationBonusTooltip(template)
{
	var popBonus = "";
	if (template.cost && template.cost.populationBonus)
		popBonus = "\n[font=\"serif-bold-13\"]Population Bonus:[/font] " + template.cost.populationBonus;
	return popBonus;
}

/**
 * Returns a message with the amount of each resource needed to create an entity.
 */
function getNeededResourcesTooltip(resources)
{
	var formatted = [];
	for (var resource in resources)
		formatted.push("[font=\"serif-12\"]" + getCostComponentDisplayName(resource) + "[/font] " + resources[resource]);

	return "\n\n[font=\"serif-bold-13\"][color=\"red\"]Insufficient resources:[/color][/font]\n" + formatted.join("  ");
}

function getEntitySpeed(template)
{
	var speed = "";
	if (template.speed)
	{
		speed += "[font=\"serif-bold-13\"]Speed:[/font] ";
		var speeds = [];
		if (template.speed.walk) speeds.push(template.speed.walk + " [font=\"serif-12\"]Walk[/font]");
		if (template.speed.run) speeds.push(template.speed.run + " [font=\"serif-12\"]Run[/font]");

		speed += speeds.join(", ");
	}
	return speed;
}

function getEntityAttack(template)
{
	var attacks = [];
	if (template.attack)
	{
		for (var type in template.attack)
		{
			var attack = "[font=\"serif-bold-13\"]" + type + " Attack:[/font] " + damageTypesToText(template.attack[type]);
			// Show max attack range if ranged attack, also convert to tiles (4m per tile)
			if (type == "Ranged")
				attack += ", [font=\"serif-bold-13\"]Range:[/font] "+Math.round(template.attack[type].maxRange/4);
			attacks.push(attack);
		}
	}
	return attacks.join("\n");
}

function getEntityName(template)
{
	return template.name.specific || template.name.generic || "???";
}

function getEntityNames(template)
{
	var names = [];
	if (template.name.specific)
	{
		names.push(template.name.specific);
		if (template.name.generic && names[0] != template.name.generic)
			names.push("(" + template.name.generic + ")");
	}
	else if (template.name.generic)
		names.push(template.name.generic);

	return (names.length) ? names.join(" ") : "???";
}

function getEntityNamesFormatted(template)
{
	return '[font="serif-bold-16"]' + getEntityNames(template) + "[/font]";
}

function getEntityRankedName(entState)
{
	var template = GetTemplateData(entState.template)
	var rank = entState.identity.rank;
	if (rank)
		return rank + " " + template.name.specific;
	else
		return template.name.specific;
}

function getRankIconSprite(entState)
{
	if ("Elite" == entState.identity.rank)
		return "stretched:session/icons/rank3.png";
	else if ("Advanced" == entState.identity.rank)
		return "stretched:session/icons/rank2.png";
	else if (entState.identity.classes && entState.identity.classes.length && -1 != entState.identity.classes.indexOf("CitizenSoldier"))
		return "stretched:session/icons/rank1.png";

	return "";
}
