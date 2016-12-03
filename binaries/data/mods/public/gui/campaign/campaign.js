// ID of the current campaign. This is the name of the json file (not the "human readable" name)
var g_CampaignID = null;

// Campaign template data from the JSON file.
var g_CampaignTemplate = null;

// name of the file we're saving campaign data in
var g_CampaignSave = null;

// Current campaign state, to be saved in/loaded from the above file
var g_CampaignData = null;

// this function is called by session.js at the end of a game. It should save the campaign save state immediately.
function campaignGameEnded(data)
{
	g_CampaignID = data.ID;
	g_CampaignTemplate = data.template;
	g_CampaignSave = data.save;
	g_CampaignData = data.data;

	// TODO: Deal with the endGameData

	if (data.endGameData.status !== "won")
	{
		if (!g_CampaignData.completed)
			g_CampaignData.completed = [];
		if (g_CampaignData.completed.indexOf(data.scenario) == -1)
			g_CampaignData.completed.push(data.scenario);
	}

	saveCampaign();
}

function hasRequirements(scenario)
{
	if (!scenario.Requires)
		return true;

	if (!g_CampaignData.completed)
		return false;

	if (scenario.Requires.some(req => g_CampaignData.completed.indexOf(req) === -1))
		return false;

	return true;
}

function hasCompleted(scenario)
{
	if (!g_CampaignData.completed)
		return false;

	if (g_CampaignData.completed.indexOf(scenario.ID) === -1)
		return false;

	return true;
}

