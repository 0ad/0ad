// ID of the current campaign. This is the name of the mod folder (not the "human readable" name)
var g_CurrentCampaign = null;

// This stores current campaign state.
var g_CampaignData = null;

function loadCurrentCampaign()
{
	let campaign = Engine.ConfigDB_GetValue("user", "currentcampaign");

	if (!campaign)
	{
		warn("No campaign chosen, currentcampaign is not defined in user.cfg. Quitting campaign mode.")
		ExitCampaignMode();
	}

	// TODO: check proper mods are loaded, proper version and so on (see savegame/load.js)	
	warn(uneval(campaign));
	loadCampaign(campaign);
}

function loadCampaign(campaign)
{
	if (g_CurrentCampaign)
	{
		warn("Campaign already loaded");
		return;
	}
	g_CurrentCampaign = campaign;

	let campaignData = Engine.LoadCampaign(g_CurrentCampaign);
	if (!campaignData)
	{
		warn("Campaign failed to load properly. Quitting campaign mode.")
		ExitCampaignMode();
	}
	g_CampaignData = campaignData.campaign_state;
}

function ExitCampaignMode()
{
	// TODO: save campaign state ?
	
	// TODO: might be safer to check all mods and remove all with type "campaign"
	let mods = getExistingModsFromConfig();
	mods.filter(mod => mod != g_CurrentCampaign);
	Engine.SetMods(mods);
	Engine.RestartEngine();
}
