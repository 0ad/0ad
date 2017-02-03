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

function hasSameRestrictionCategory(templateName1, templateName2)
{
	let template1 = GetTemplateData(templateName1);
	let template2 = GetTemplateData(templateName2);

	if (template1.trainingRestrictions && template2.trainingRestrictions)
		return template1.trainingRestrictions.category == template2.trainingRestrictions.category;
	if (template1.buildRestrictions && template2.buildRestrictions)
		return template1.buildRestrictions.category == template2.buildRestrictions.category;
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
	let entState = GetEntityState(ent);
	if (entState && !entState.position && entState.unitAI && entState.unitAI.orders.length &&
			(entState.unitAI.orders[0].type == "Garrison" || entState.unitAI.orders[0].type == "Autogarrison"))
		return getEntityOrHolder(entState.unitAI.orders[0].data.target);

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

/**
 * Send the current list of players, teams, AIs, observers and defeated/won and offline states to the lobby.
 * The playerData format from g_GameAttributes is kept to reuse the GUI function presenting the data.
 */
function sendLobbyPlayerlistUpdate()
{
	if (!g_IsController || !Engine.HasXmppClient())
		return;

	// Extract the relevant player data and minimize packet load
	let minPlayerData = [];
	for (let playerID in g_GameAttributes.settings.PlayerData)
	{
		if (+playerID == 0)
			continue;

		let pData = g_GameAttributes.settings.PlayerData[playerID];

		let minPData = { "Name": pData.Name };

		if (g_GameAttributes.settings.LockTeams)
			minPData.Team = pData.Team;

		if (pData.AI)
		{
			minPData.AI = pData.AI;
			minPData.AIDiff = pData.AIDiff;
		}

		if (g_Players[playerID].offline)
			minPData.Offline = true;

		// Whether the player has won or was defeated
		let state = g_Players[playerID].state;
		if (state != "active")
			minPData.State = state;

		minPlayerData.push(minPData);
	}

	// Add observers
	let connectedPlayers = 0;
	for (let guid in g_PlayerAssignments)
	{
		let pData = g_GameAttributes.settings.PlayerData[g_PlayerAssignments[guid].player];

		if (pData)
			++connectedPlayers;
		else
			minPlayerData.push({
				"Name": g_PlayerAssignments[guid].name,
				"Team": "observer"
			});
	}

	Engine.SendChangeStateGame(connectedPlayers, playerDataToStringifiedTeamList(minPlayerData));
}
