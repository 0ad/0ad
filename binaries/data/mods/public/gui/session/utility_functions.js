function getPlayerData(previousData = undefined)
{
	let players = [];

	let simState = GetSimState();
	if (!simState)
		return players;

	for (let i = 0; i < simState.players.length; ++i)
	{
		let playerState = simState.players[i];
		players.push({
		    "name": playerState.name,
		    "civ": playerState.civ,
		    "color": {
			    "r": playerState.color.r*255,
			    "g": playerState.color.g*255,
			    "b": playerState.color.b*255,
			    "a": playerState.color.a*255
			},
		    "team": playerState.team,
		    "teamsLocked": playerState.teamsLocked,
		    "cheatsEnabled": playerState.cheatsEnabled,
		    "state": playerState.state,
		    "isAlly": playerState.isAlly,
		    "isMutualAlly": playerState.isMutualAlly,
		    "isNeutral": playerState.isNeutral,
		    "isEnemy": playerState.isEnemy,
		    "guid": undefined, // network guid for players controlled by hosts
		    "offline": previousData && !!previousData[i].offline
		});
	}

	if (g_PlayerAssignments)
		for (let guid in g_PlayerAssignments)
		{
			let playerID = g_PlayerAssignments[guid].player;

			if (!players[playerID])
				continue;

			players[playerID].guid = guid;
			players[playerID].name = g_PlayerAssignments[guid].name;
		}

	return players;
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
	if (!gain)
		return "";

	var simState = GetSimState();

	var gainString = gain.traderGain;
	if (gain.market1Gain && gain.market1Owner == gain.traderOwner)
		gainString += translate("+") + gain.market1Gain;
	if (gain.market2Gain && gain.market2Owner == gain.traderOwner)
		gainString += translate("+") + gain.market2Gain;

	var tooltip = sprintf(translate("%(gain)s (%(player)s)"), {
		"gain": gainString,
		"player": simState.players[gain.traderOwner].name
	});

	if (gain.market1Gain && gain.market1Owner != gain.traderOwner)
		tooltip += translateWithContext("Separation mark in an enumeration", ", ") + sprintf(translate("%(gain)s (%(player)s)"), {
			"gain": gain.market1Gain,
			"player": simState.players[gain.market1Owner].name
		});

	if (gain.market2Gain && gain.market2Owner != gain.traderOwner)
		tooltip += translateWithContext("Separation mark in an enumeration", ", ") + sprintf(translate("%(gain)s (%(player)s)"), {
			"gain": gain.market2Gain,
			"player": simState.players[gain.market2Owner].name
		});

	return tooltip;
}

/**
 * Returns the entity itself except when garrisoned where it returns its garrisonHolder
 */
function getEntityOrHolder(ent)
{
	var entState = GetEntityState(ent);
	if (entState && !entState.position && entState.unitAI && entState.unitAI.orders.length &&
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
