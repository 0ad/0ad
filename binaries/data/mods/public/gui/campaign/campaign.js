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
