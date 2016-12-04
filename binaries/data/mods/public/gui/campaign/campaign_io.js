// This file provides helper functions related to loading/saving campaign save states and campaign templates
// Most functions rely on loading campaign.js, but not all (such as canLoadCurrentCampaign)

/*
 * TODOs in this file:
 * - Provide a way to check if the campaign state changed?
 * Other things
 */

function LoadAvailableCampaigns()
{
	let campaigns = Engine.BuildDirEntList("campaigns/", "*.json", false);

	let ret = {};

	for (let filename of campaigns)
	{
		let data = Engine.ReadJSONFile(filename);
		if (!data)
			continue;

		// TODO: sanity checks, probably in their own function?
		if (!data.Name)
			continue;

		ret[filename.replace("campaigns/","").replace(".json","")] = data;
	}

	return ret;
}

function canLoadCurrentCampaign(verbose = false)
{
	let campaign = Engine.ConfigDB_GetValue("user", "currentcampaign");

	if (!campaign)
	{
		if (verbose)
			warn("No campaign chosen, currentcampaign is not defined in user.cfg. Quitting campaign mode.")
		return false;
	}

	if (!Engine.FileExists("campaignsaves/" + campaign + ".0adcampaign"))
	{
		if (verbose)
			warn("Current campaign not found. Quitting campaign mode.");
		return false;
	}

	// TODO: load up the file and do some checks?

	return true;
}

function loadCurrentCampaignSave()
{
	let campaign = Engine.ConfigDB_GetValue("user", "currentcampaign");

	if (!canLoadCurrentCampaign(true))
		return false;

	if (g_CampaignSave)
	{
		warn("Campaign already loaded");
		return false;
	}
	g_CampaignSave = campaign;

	let campaignData = Engine.ReadJSONFile("campaignsaves/" + g_CampaignSave + ".0adcampaign");
	if (!campaignData)
	{
		warn("Campaign failed to load properly. Quitting campaign mode.")
		return false;
	}

	g_CampaignData = campaignData;
	g_CampaignID = g_CampaignData.campaign;

	if (!loadCampaignTemplate(g_CampaignID))
		return false;

	// actually fetch the menu
	Engine.SwitchGuiPage(g_CampaignTemplate.Interface, {"ID" : g_CampaignID, "template" : g_CampaignTemplate, "save": g_CampaignSave, "data" : g_CampaignData});

	return true;
}

function loadCampaignTemplate(name)
{
	let data = Engine.ReadJSONFile("campaigns/" + name + ".json");

	if (!data)
	{
		warn("Could not parse campaign data.");
		return false;
	}
	
	g_CampaignTemplate = data;

	return true;
}

function saveCampaign()
{
	if (!g_CampaignSave)
	{
		warn("Cannot save campaign, no campaign is currently loaded");
		return false;
	}

	Engine.WriteJSONFile("campaignsaves/" + g_CampaignSave + ".0adcampaign", g_CampaignData);

	return true;
}
