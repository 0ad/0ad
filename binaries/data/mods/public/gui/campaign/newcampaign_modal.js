var g_CampaignID = null;
var g_CampaignData = null;

function init(data)
{
	// TODO: load up existing campaign in the dropdown above
	Engine.GetGUIObjectByName("saveGameDesc").caption = data.campaignID;
	g_CampaignID = data.campaignID;
	g_CampaignData = data.campaignData;
}

function startCampaign()
{
	// TODO: handle overwrite and so on

	// temp: prefill campaign name
	realStartCampaign(Engine.GetGUIObjectByName("saveGameDesc").caption);
}

function realStartCampaign(name)
{
	Engine.SaveCampaign(name, {"name" : name, "campaign" : g_CampaignID});

	// inform user config that we are playing this campaign
	Engine.ConfigDB_CreateValue("user", "currentcampaign", name);
	Engine.ConfigDB_WriteValueToFile("user", "currentcampaign", name, "config/user.cfg");

	loadCurrentCampaignSave();
}