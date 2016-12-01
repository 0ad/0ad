var g_CurrentCampaign = null;
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

}
