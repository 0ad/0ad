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
		gameSelection.list = campaigns.map(path => generateLabel(pathToGame(path)));
		gameSelection.list_data = campaigns.map(path => pathToGame(path));
	}

	if (data)
	{
		g_CampaignID = data.campaignID;
		g_CampaignData = data.campaignData;
	}
}

function selectionChanged()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	if (gameSelection.selected === -1)
		return;

	// TODO: do something?
}

function startCampaign()
{
	// TODO: handle overwrite and so on

	// temp: prefill campaign name
	realStartCampaign(Engine.GetGUIObjectByName("saveGameDesc").caption);
}

function realStartCampaign(desc)
{
	let name = g_CampaignID + "_1";
	// if file already exists, pick the number above the existing ones. Don't bother making it dense.
	if (Engine.FileExists("campaignsaves/" + name + ".0adcampaign"))
	{
		// get other campaigns following that template
		let campaigns = Engine.BuildDirEntList("campaignsaves/", g_CampaignID + "_*.0adcampaign", false);
		let max = 1;
		for (let camp of campaigns)
		{
			let nb = camp.replace("campaignsaves/" + g_CampaignID + "_","").replace(".0adcampaign","");
			if (+nb > max)
				max = +nb;
		}
		name = g_CampaignID + "_" + (max+1);
		// sanity check
		if (Engine.FileExists("campaignsaves/" + name + ".0adcampaign"))
		{
			error("tell wraitii he can't code");
			return;
		}
	}

	saveCampaign(name, {"userDescription" : desc, "campaign" : g_CampaignID})

	// inform user config that we are playing this campaign
	Engine.ConfigDB_CreateValue("user", "currentcampaign", name);
	Engine.ConfigDB_WriteValueToFile("user", "currentcampaign", name, "config/user.cfg");

	loadCurrentCampaignSave();
}