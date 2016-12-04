var g_CampaignID = null;
var g_CampaignData = null;

// TODO: refuse empty names

function init(data)
{
	// load existing campaigns
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	gameSelection.selected = -1;

	let campaigns = Engine.BuildDirEntList("campaignsaves/", "*.0adcampaign", false);
	if (!campaigns.length)
		gameSelection.list = [translate("No ongoing campaigns.")];
	else
	{
		gameSelection.list = campaigns.map(path => path.replace("campaignsaves/",""));
		gameSelection.list_data = campaigns.map(path => pathToGame(path));
	}

	g_CampaignID = data.campaignID;
	g_CampaignData = data.campaignData;
}

function selectionChanged()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	if (gameSelection.selected === -1)
		return;

	Engine.GetGUIObjectByName("saveGameDesc").caption = gameSelection.list_data[gameSelection.selected];
}

function startCampaign()
{
	// TODO: handle overwrite and so on

	// temp: prefill campaign name
	realStartCampaign(Engine.GetGUIObjectByName("saveGameDesc").caption);
}

function realStartCampaign(name)
{
	Engine.WriteJSONFile("campaignsaves/" + name + ".0adcampaign", {"name" : name, "campaign" : g_CampaignID});

	// inform user config that we are playing this campaign
	Engine.ConfigDB_CreateValue("user", "currentcampaign", name);
	Engine.ConfigDB_WriteValueToFile("user", "currentcampaign", name, "config/user.cfg");

	loadCurrentCampaignSave();
}