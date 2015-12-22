// Get the basic player data
function getPlayerData(playerAssignments)
{
	var players = [];

	var simState = GetSimState();
	if (!simState)
		return players;

	for (var i = 0; i < simState.players.length; i++)
	{
		var playerState = simState.players[i];

		var name = playerState.name;
		var civ = playerState.civ;
		var color = {
		    "r": playerState.color.r*255,
		    "g": playerState.color.g*255,
		    "b": playerState.color.b*255,
		    "a": playerState.color.a*255
		};

		var player = {
		    "name": name,
		    "civ": civ,
		    "color": color,
		    "team": playerState.team,
		    "teamsLocked": playerState.teamsLocked,
		    "cheatsEnabled": playerState.cheatsEnabled,
		    "state": playerState.state,
		    "isAlly": playerState.isAlly,
		    "isMutualAlly": playerState.isMutualAlly,
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

function findGuidForPlayerID(playerID)
{
	return Object.keys(g_PlayerAssignments).find(guid => g_PlayerAssignments[guid].player == playerID);
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
	// note: use the functions in globalscripts/Templates.js for more versatile matching
	if (entState.identity)
	{
		var classes = entState.identity.classes;
		if (classes && classes.length)
			return (classes.indexOf(className) != -1);
	}
	return false;
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

/**
 * Returns a message with the details of the trade gain.
 */
function getTradingTooltip(gain)
{

	var playerID = Engine.GetPlayerID();
	var simState = GetSimState();
	
	var gainString = gain.traderGain;
	if (gain.market1Gain && gain.market1Owner == gain.traderOwner)
		gainString += translate("+") + gain.market1Gain;
	if (gain.market2Gain && gain.market2Owner == gain.traderOwner)
		gainString += translate("+") + gain.market2Gain;

	var tooltip = sprintf(translate("%(gain)s (%(player)s)"), {
		gain: gainString,
		player: (!g_IsNetworked && gain.traderOwner == playerID) ? translate("You") : simState.players[gain.traderOwner].name
	});
	
	if (gain.market1Gain && gain.market1Owner != gain.traderOwner)
		tooltip += translateWithContext("Separation mark in an enumeration", ", ") + sprintf(translate("%(gain)s (%(player)s)"), {
			gain: gain.market1Gain,
			player: (!g_IsNetworked && gain.market1Owner == playerID) ? translate("You") : simState.players[gain.market1Owner].name
		});
	if (gain.market2Gain && gain.market2Owner != gain.traderOwner)
		tooltip += translateWithContext("Separation mark in an enumeration", ", ") + sprintf(translate("%(gain)s (%(player)s)"), {
			gain: gain.market2Gain,
			player: (!g_IsNetworked && gain.market2Owner == playerID) ? translate("You") : simState.players[gain.market2Owner].name
		});

	return tooltip;
}

/**
 * Returns the entity itself except when garrisoned where it returns its garrisonHolder
 */
function getEntityOrHolder(ent)
{
	var entState = GetEntityState(ent);
	if (entState && !entState.position && entState.unitAI && entState.unitAI.orders.length > 0 &&
			(entState.unitAI.orders[0].type == "Garrison" || entState.unitAI.orders[0].type == "Autogarrison"))
		return entState.unitAI.orders[0].data.target;

	return ent;
}

/**
 * Returns a "color:255 0 0 Alpha" string based on how many resources are needed.
 */
function resourcesToAlphaMask(neededResources)
{
	var totalCost = 0;
	for (var resource in neededResources)
		totalCost += +neededResources[resource];
	var alpha = 50 + Math.round(+totalCost/10.0);
	alpha = alpha > 125 ? 125 : alpha;
	return "color:255 0 0 " + alpha;
}
