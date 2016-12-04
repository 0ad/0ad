// ID of the current campaign. This is the name of the json file (not the "human readable" name)
var g_CampaignID = null;

// Campaign template data from the JSON file.
var g_CampaignTemplate = null;

// name of the file we're saving campaign data in
var g_CampaignSave = null;

// Current campaign state, to be saved in/loaded from the above file
var g_CampaignData = null;

function startLevel(level)
{
	let matchID = launchGame(g_CampaignTemplate.Levels[level], level);

	g_CampaignData.currentlyPlaying = matchID;

	saveCurrentCampaign();
}

// this function is called by session.js at the end of a game. It should save the campaign save state immediately.
function campaignGameEnded(data)
{
	g_CampaignID = data.ID;
	g_CampaignTemplate = data.template;
	g_CampaignSave = data.save;
	g_CampaignData = data.data;

	// TODO: Deal with the endGameData

	// if we're not active, we either lost or won, so we're no longer playing the game.
	if (data.endGameData.status !== "active")
	{
		if (g_CampaignData.currentlyPlaying)
			g_CampaignData.currentlyPlaying = undefined;
	}
	if (data.endGameData.status === "won")
	{
		if (!g_CampaignData.completed)
			g_CampaignData.completed = [];
		if (g_CampaignData.completed.indexOf(data.level) == -1)
			g_CampaignData.completed.push(data.level);
	}

	saveCurrentCampaign();
}

// returns true if the level "level" is available.
function hasRequirements(level)
{
	if (!level.Requires)
		return true;

	if (!g_CampaignData.completed)
		return false;

	// reuse class matching system, supporting "or", "+" and "!"
	if (!MatchesClassList(g_CampaignData.completed, level.Requires))
		return false;

	return true;
}

// return true if the player has completed the level with ID "level"
function hasCompleted(level)
{
	if (!g_CampaignData.completed)
		return false;

	if (g_CampaignData.completed.indexOf(level.ID) === -1)
		return false;

	return true;
}

