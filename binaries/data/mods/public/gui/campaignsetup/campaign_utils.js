// ID of the current campaign. This is the name of the mod folder (not the "human readable" name)
var g_CurrentCampaignID = null;

// name of the campaign file currently loaded
var g_CurrentCampaign = null;

// (optional) campaign data loaded from XML or JSON, to make modder's life easier.
var g_CampaignTemplate = null;

// saves mods, for convenience on continue
var g_CampaignMods = null;

// This stores current campaign state.
var g_CampaignData = null;

/*
 * TODOs in this file:
 * - Provide a way to check if the campaign state changed?
 * Other things
 */

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

	// TODO: check proper mods are loaded, proper version and so on (see savegame/load.js)	

	return true;
}

function loadCurrentCampaignState()
{
	let campaign = Engine.ConfigDB_GetValue("user", "currentcampaign");

	if (!canLoadCurrentCampaign(true))
		exitCampaignMode();

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
		exitCampaignMode();
	}

	g_CampaignData = campaignData.campaign_state;
	g_CampaignMods = campaignData.mods;
	g_CurrentCampaignID = g_CampaignData.campaign;

	loadCampaignTemplate();
}

// to easily create new campaigns without going in the nitty gritty of the JS, an XML/JSON file may be provided.
// This functions loads that to g_CampaignTemplate.
function loadCampaignTemplate()
{
	let ret;
	if (Engine.FileExists("campaign/campaign.xml"))
		ret = Engine.LoadCampaignTemplateXML();
	else if (Engine.FileExists("campaign/campaign.json"))
		ret = Engine.LoadCampaignTemplateJSON();
	else
		return;

	if (!ret)
	{
		warn("Campaign template could not be loaded");
		return;
	}

	g_CampaignTemplate = ret;
}

function saveCampaign()
{
	if (!g_CurrentCampaign)
	{
		warn("Cannot save campaign, no campaign is currently loaded");
		return;
	}

	Engine.SaveCampaign(g_CurrentCampaign, g_CampaignData);
}

function exitCampaignMode(exitGame = false)
{
	// TODO: should this be here?
	saveCampaign();

	if (exitGame)
	{
		messageBox(
		400, 200,
		translate("Are you sure you want to quit 0 A.D.?"),
		translate("Confirmation"),
		[translate("No"), translate("Yes")],
		[null, Engine.Exit]
		);
		return;
	}

	// TODO: might be safer to check all mods and remove all with type "campaign"
	let mods = getExistingModsFromConfig();
	mods.filter(mod => mod != g_CurrentCampaignID);
	Engine.SetMods(mods);
	Engine.RestartEngine();
}
