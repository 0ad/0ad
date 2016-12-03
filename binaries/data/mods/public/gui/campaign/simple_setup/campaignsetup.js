var g_CampaignsAvailable = {}; // "name of JSON file/ID of campaign" : data as parsed JSON

var g_SelectedCampaign = null;

/*
 * Initializes the campaign window.
 * Loads all campaigns
 * Allows you to start them.
 */

// TODO: Should we support mods?

function init(data)
{
	g_CampaignsAvailable = LoadAvailableCampaigns();

	GenerateCampaignList();
}

function GenerateCampaignList()
{
	// TODO: Remember previously selected
	let selection = Engine.GetGUIObjectByName("campaignSelection");
	//if (oldSelection.selected != -1)
	//	g_SelectedReplayDirectory = g_ReplaysFiltered[replaySelection.selected].directory;

	let list = [];
	for (let key in g_CampaignsAvailable)
		list.push({ "directories" : key, "name" : g_CampaignsAvailable[key].Name, "difficulty" : g_CampaignsAvailable[key].Difficulty });

	// change array of object into object of array.
	list = prepareForDropdown(list);

	// Push to GUI
	selection.selected = -1;
	selection.list_name = list.name || [];
	selection.list_difficulty = list.difficulty || [];

	// Change these last, otherwise crash
	// TODO: do we need both of those? I'm unsure.
	selection.list = list.directories || [];
	selection.list_data = list.directories || [];

//	replaySelection.selected = replaySelection.list.findIndex(directory => directory == g_SelectedReplayDirectory);

//	displayReplayDetails();
}

function displayCampaignDetails()
{
	// TODO: basically all of it.
	let selection = Engine.GetGUIObjectByName("campaignSelection");
	if (selection.selected === -1)
		return;

	g_SelectedCampaign = selection.list[selection.selected];

	Engine.GetGUIObjectByName("startCampButton").enabled = true;
}

function startCampaign()
{
	Engine.PushGuiPage("page_newcampaign.xml", { "campaignID" : g_SelectedCampaign, "campaignData" : g_CampaignsAvailable[g_SelectedCampaign] });
}
