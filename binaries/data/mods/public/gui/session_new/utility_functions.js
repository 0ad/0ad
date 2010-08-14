//-------------------------------- -------------------------------- -------------------------------- 
// Utility functions
//-------------------------------- -------------------------------- -------------------------------- 

function toTitleCase(string)
{
	if (string.length > 0)
		string = string.charAt(0).toUpperCase() + string.substring(1, string.length).toLowerCase();
	return string;
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
		var color = {"r": 255, "g": 255, "b": 255, "a": 255};
		color.r = playerState.color["r"]*255;
		color.g = playerState.color["g"]*255;
		color.b = playerState.color["b"]*255;
		color.a = playerState.color["a"]*255;

		var player = {"name": name, "civ": civ, "color": color};
		players.push(player);
	}
	
	if (playerAssignments)
	{
		for each (var playerAssignment in playerAssignments)
		{
			if (players[playerAssignment.player])
				players[playerAssignment.player].name = playerAssignment.name;
		}
	}
	
	return players;
}

function isUnit(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
			for (var i = 0; i < classes.length; i++)
				if ((classes[i] == "Organic") || (classes[i] == "Mechanical"))
					return true;
	}
	return false;
}

function isDefensive(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
		for (var i = 0; i < classes.length; i++)
			if (classes[i] == "Defensive")
				return true;
	}
	return false;
}

function damageTypesToTextStacked(dmg)
{
	if (!dmg)
		return "(None)";
	return dmg.hack + " Hack\n" + dmg.pierce + " Pierce\n" + dmg.crush + " Crush";
}

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

	return dmgArray.join(", ");
}

function getCommandCellId(commandName)
{
	switch (commandName)
	{
	case "delete":
		return 4;
	default:
		return -1;
	}
}

function getEntityCommandsList(entState)
{
	var commands = [];
	commands.push("delete");
	return commands;
}

function getRankCellId(entState)
{
	if (entState.template)
	{
		var templateName = entState.template;
		var endsWith = templateName.substring(templateName.length-2, templateName.length);
	
		if (isUnit(entState))
		{
			if (endsWith == "_e")
				return 0;
			else if (endsWith == "_a")
				return 1;
		}
	}
	return -1;
}

function getRankTitle(cellId)
{
	if (cellId == 0)
		return " (Elite)";
	else if (cellId == 1)
		return " (Advanced)";
	return "";
}

function getEntityCost(template)
{
	if (template.cost)
	{
		var costs = [];
		if (template.cost.food) costs.push("[font=\"serif-bold-13\"]Food:[/font] " + template.cost.food);
		if (template.cost.wood) costs.push("[font=\"serif-bold-13\"]Wood:[/font] " + template.cost.wood);
		if (template.cost.metal) costs.push("[font=\"serif-bold-13\"]Metal:[/font] " + template.cost.metal);
		if (template.cost.stone) costs.push("[font=\"serif-bold-13\"]Stone:[/font] " + template.cost.stone);
		if (costs.length)
			return costs.join(", ");
	}
	return "";
}

function getEntityName(template)
{
		return "[font=\"serif-bold-16\"]" + (template.name.specific || template.name.generic || "???") + "[/font]";
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

function getFormalCivName(civ)
{
	switch (civ)
	{
	case CART:
		return "Carthaginians";
	case CELT:
		return "Celts";
	case HELE:
		return "Hellenes";
	case IBER:
		return "Iberians";
	case PERS:
		return "Persians";
	case ROME:
		return "Romans";
	default:
		return "Gaia";
	}
}
