const GEOLOGY = "geology";
const FLORA = "flora";
const FAUNA = "fauna";
const SPECIAL = "special";

//-------------------------------- -------------------------------- -------------------------------- 
// Session Dialog (only one at a time)
//-------------------------------- -------------------------------- -------------------------------- 
var g_SessionDialog = new SessionDialog();

function SessionDialog()
{
	this.referencedPanel = {};
}

SessionDialog.prototype.open = function(title, message, referencedPanel, x, y, confirmFunction)
{
	// hide previous panel referencedPanel if applicable
	if (this.referencedPanel)
		this.referencedPanel.hidden = true

	// set dialog title if applicable
	getGUIObjectByName("sessionDialogTitle").caption = title? title : "";

	// set dialog message if applicable
	getGUIObjectByName("sessionDialogMessage").caption = message? message : "";

	// set panel reference if applicable
	if(referencedPanel)
	{
		referencedPanel.size =  "50%-" + ((x/2)-30) + " 50%-" + ((y/2)-30) + " 50%+" + ((x/2)-30) + " 50%+" + ((y/2)-72);
		referencedPanel.hidden = false;
		this.referencedPanel = referencedPanel;
	}

	// set confirm function if applicable
	if (confirmFunction)
	{
		var buttonFunction = function () {
			this.close(referencedPanel); // "this" is defined as SessionDialog in this context
			confirmFunction();
		};

		var dialog = this;
		var confirmButton = getGUIObjectByName("sessionDialogConfirm");
		confirmButton.onpress = function() { buttonFunction.call(dialog); };
		confirmButton.hidden = false;
		confirmButton.size = "32 100%-56 144 100%-24";
		getGUIObjectByName("sessionDialogCancel").size = "100%-144 100%-56 100%-32 100%-24";
		getGUIObjectByName("sessionDialogCancel").caption = "Cancel";
	}
	else
	{
		getGUIObjectByName("sessionDialogConfirm").hidden = true;
		getGUIObjectByName("sessionDialogCancel").size = "50%-56 100%-56 50%+56 100%-24";
		getGUIObjectByName("sessionDialogCancel").caption = "Close";
	}

	getGUIObjectByName("sessionDialog").size = "50%-" + x/2 + " 50%-" + y/2 + " 50%+" + x/2 + " 50%+" + y/2;
	getGUIObjectByName("sessionDialog").hidden = false;
};


SessionDialog.prototype.close = function()
{
	getGUIObjectByName("sessionDialog").hidden = true;
	if (this.referencedPanel)
		this.referencedPanel.hidden = true;
};

//-------------------------------- -------------------------------- -------------------------------- 
// Utility functions
//-------------------------------- -------------------------------- -------------------------------- 

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
			"diplomacy": playerState.diplomacy,
			"state": playerState.state,
			"guid": undefined, // network guid for players controlled by hosts
			"disconnected": false, // flag for host-controlled players who have left the game
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

// Update player data when a host has connected
function updatePlayerDataAdd(players, hostGuid, playerAssignment)
{
	if (players[playerAssignment.player])
	{
		players[playerAssignment.player].guid = hostGuid;
		players[playerAssignment.player].name = playerAssignment.name;
	}
}

// Update player data when a host has disconnected
function updatePlayerDataRemove(players, hostGuid)
{
	for each (var player in players)
		if (player.guid == hostGuid)
			player.offline = true;
}

function isUnit(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
			for (var i = 0; i < classes.length; i++)
				if (classes[i] == "Unit")
					return true;
	}
	return false;
}

function isAnimal(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
		for (var i = 0; i < classes.length; i++)
			if (classes[i] == "Animal")
				return true;
	}
	return false;
}

function isStructure(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
		for (var i = 0; i < classes.length; i++)
			if (classes[i] == "Structure")
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

	return dmgArray.join("[font=\"serif-12\"], [/font]");
}

function getFormationCellId(formationName)
{
	switch (formationName)
	{
	case "Formation0":
		return 0;
	case "Formation1":
		return 1;
	case "Formation2":
		return 2;
	case "Formation3":
		return 3;
	case "Formation4":
		return 4;
	case "Formation5":
		return 5;
	case "Formation6":
		return 6;
	case "Formation7":
		return 7;
	case "Formation8":
		return 8;
	case "Formation9":
		return 9;
	case "Formation10":
		return 10;	
	case "Formation11":
		return 11;
	case "Formation12":
		return 12;
	default:
		return -1;
	}
}

function getCommandImage(commandName)
{
	switch (commandName)
	{
	case "delete":
		return "kill_small.png";
	case "unload-all":
		return "garrison.png";
	default:
		return "";
	}
}

function getEntityFormationsList(entState)
{
	var formations = [];

	formations.push("Formation0");
	formations.push("Formation1");
	formations.push("Formation2");
	formations.push("Formation3");
	formations.push("Formation4");
	formations.push("Formation5");
	formations.push("Formation6");
	formations.push("Formation7");
	formations.push("Formation8");
	formations.push("Formation9");
	formations.push("Formation10");
	formations.push("Formation11");
	formations.push("Formation12");
	return formations;
}

function getEntityCommandsList(entState)
{
	var commands = [];
	if (entState.garrisonHolder)
		commands.push("unload-all");
	commands.push("delete");
	return commands;
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
		if (template.cost.population) costs.push("[font=\"serif-bold-13\"]Population:[/font] " + template.cost.population);

		if (costs.length)
			return costs.join(", ");
	}
	return "";
}

function getPopulationBonus(template)
{
	var popBonus = "";
	if (template.cost.populationBonus)
		popBonus = "[font=\"serif-bold-13\"]Population Bonus:[/font] " + template.cost.populationBonus;
	return popBonus;
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

function getRankIconCellId(entState)
{
	var template = GetTemplateData(entState.template)
	var rank = entState.identity.rank;
	if (rank)
	{
		if (rank == "Elite")
			return 0;
		else if (rank == "Advanced")
			return 1;
	}
	return -1;
}
