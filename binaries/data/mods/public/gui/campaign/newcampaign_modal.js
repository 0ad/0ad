var g_Campaign = null;

function init(data)
{
	// TODO: load up existing campaign in the dropdown above
	Engine.GetGUIObjectByName("saveGameDesc").caption = data.campaign;
	g_Campaign = data.campaign;
}

function startCampaign()
{
	// TODO: handle overwrite
	realStartCampaign(Engine.GetGUIObjectByName("saveGameDesc").caption, g_Campaign);
}

function realStartCampaign(name, campaign)
{
	Engine.SaveCampaign(name, {"name" : name, "campaign" : campaign});

	// inform user config that we are playing this campaign
	Engine.ConfigDB_CreateValue("user", "currentcampaign", name);
	Engine.ConfigDB_WriteValueToFile("user", "currentcampaign", name, "config/user.cfg");

	// load up the campaign mod and restart.
	// TODO: make sure getExistingModsFromConfig returns mod in proper dependency order (though it ought to)
	let mods = getExistingModsFromConfig().concat(campaign);
	Engine.SetMods(mods);
	Engine.RestartEngine();
}