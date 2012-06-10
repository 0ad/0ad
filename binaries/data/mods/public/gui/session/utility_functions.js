const GEOLOGY = "geology";
const FLORA = "flora";
const FAUNA = "fauna";
const SPECIAL = "special";

const COST_DISPLAY_NAMES = {
	"food": "Food",
	"wood": "Wood",
	"stone": "Stone",
	"metal": "Metal",
	"population": "Population"
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
		var color = {"r": playerState.colour.r*255, "g": playerState.colour.g*255, "b": playerState.colour.b*255, "a": playerState.colour.a*255};

		var player = {
			"name": name,
			"civ": civ,
			"color": color,
			"team": playerState.team,
			"state": playerState.state,
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
	    if (dmg.hack) dmgArray.push(dmg.hack + "[font=\"sans-10\"][color=\"orange\"]H[/color][/font]");
	    if (dmg.pierce) dmgArray.push(dmg.pierce + "[font=\"sans-10\"][color=\"orange\"]P[/color][/font]");
	    if (dmg.crush) dmgArray.push(dmg.crush + "[font=\"sans-10\"][color=\"orange\"]C[/color][/font]");
	    
	    return dmgArray.join("[font=\"serif-12\"], [/font]");
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
function getEntityCostComponentsTooltipString(template)
{
	var costs = [];
	if (template.cost.food) costs.push(template.cost.food + " [font=\"serif-12\"]" + getCostComponentDisplayName("food") + "[/font]");
	if (template.cost.wood) costs.push(template.cost.wood + " [font=\"serif-12\"]" + getCostComponentDisplayName("wood") + "[/font]");
	if (template.cost.metal) costs.push(template.cost.metal + " [font=\"serif-12\"]" + getCostComponentDisplayName("metal") + "[/font]");
	if (template.cost.stone) costs.push(template.cost.stone + " [font=\"serif-12\"]" + getCostComponentDisplayName("stone") + "[/font]");
	if (template.cost.population) costs.push(template.cost.population + " [font=\"serif-12\"]" + getCostComponentDisplayName("population") + "[/font]");
	return costs;
}

/**
 * Returns the cost information to display in the specified entity's construction button tooltip.
 */
function getEntityCostTooltip(template)
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
		
		// TODO: the costs of the wall segments should be the same, and for now we will assume they are (ideally we
		// should take the average here or something).
		var wallCosts = getEntityCostComponentsTooltipString(templateLong);
		var towerCosts = getEntityCostComponentsTooltipString(templateTower);
		
		cost += "\n";
		cost += " Walls:  " + wallCosts.join(", ") + "\n";
		cost += " Towers: " + towerCosts.join(", ");
	}
	else if (template.cost)
	{
		var costs = getEntityCostComponentsTooltipString(template);
		cost += costs.join(", ");
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
			attacks.push("[font=\"serif-bold-13\"]" + type + " Attack:[/font] " + damageTypesToText(template.attack[type]));
		}
	}
	return attacks.join("\n");
}

function getEntityName(template)
{
	return template.name.specific || template.name.generic || "???";
}

function getEntityNameWithGenericType(template)
{
	var name;
	if ((template.name.specific && template.name.generic) && (template.name.specific != template.name.generic))
		name = template.name.specific + " (" + template.name.generic + ")";
	else
		name = template.name.specific || template.name.generic || "???";

	return "[font=\"serif-bold-16\"]" + name + "[/font]";
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
	if (entState.identity && entState.identity.rank && entState.identity.classes)
	{
		if ("Elite" == entState.identity.rank)
		{
			return "stretched:session/icons/rank3.png";
		}
		else if ("Advanced" == entState.identity.rank)
		{
			return "stretched:session/icons/rank2.png";
		}
		else if (entState.identity.classes &&
			entState.identity.classes.length &&
			-1 != entState.identity.classes.indexOf("CitizenSoldier"))
		{
			return "stretched:session/icons/rank1.png";
		}
	}

	return "";
}
