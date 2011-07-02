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
			return (classes.indexOf("Unit") != -1);
	}
	return false;
}

function isAnimal(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
			return (classes.indexOf("Animal") != -1);
	}
	return false;
}

function isStructure(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
			return (classes.indexOf("Structure") != -1);
	}
	return false;
}

function isDefensive(entState)
{
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
			return (classes.indexOf("Defensive") != -1);
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
	case "Loose":
		return 0;
	case "Box":
		return 1;
	case "Column Closed":
		return 2;
	case "Line Closed":
		return 3;
	case "Column Open":
		return 4;
	case "Line Open":
		return 5;
	case "Flank":
		return 6;
	case "Skirmish":
		return 7;
	case "Wedge":
		return 8;
	case "Testudo":
		return 9;
	case "Phalanx":
		return 10;
	case "Syntagma":
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
		return "garrison-out.png";
	case "garrison":
		return "garrison.png";
	case "repair":
		return "repair.png";
	default:
		return "";
	}
}

function getEntityFormationsList(entState)
{
	var civ = g_Players[entState.player].civ;
	var formations = getCivFormations(civ);
	return formations;
}

function getCivFormations(civ)
{
	// TODO: this should come from the civ JSON files instead

	var civFormations = ["Loose", "Box", "Column Closed", "Line Closed", "Column Open", "Line Open", "Flank", "Skirmish", "Wedge", "Formation12"];
	if (civ == "hele")
	{
		civFormations.push("Phalanx");
		civFormations.push("Syntagma");
	}
	else if (civ == "rome")
	{
		civFormations.push("Testudo");
	}
	return civFormations;
}

function getEntityCommandsList(entState)
{
	var commands = [];
	if (entState.garrisonHolder)
		commands.push("unload-all");
	commands.push("delete");
	if (isUnit(entState))
		commands.push("garrison");
	if (entState.buildEntities)
		commands.push("repair");
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
		popBonus = "\n[font=\"serif-bold-13\"]Population Bonus:[/font] " + template.cost.populationBonus;
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

function getRankIconSprite(entState)
{
	if (entState.identity && entState.identity.rank && entState.identity.classes)
	{
		if ("Elite" == entState.identity.rank)
		{
			return "stretched:session/icons/single/rank3.png";
		}
		else if ("Advanced" == entState.identity.rank)
		{
			return "stretched:session/icons/single/rank2.png";
		}
		else if (entState.identity.classes &&
			entState.identity.classes.length &&
			-1 != entState.identity.classes.indexOf("CitizenSoldier"))
		{
			return "stretched:session/icons/single/rank1.png";
		}
	}

	return "";
}
