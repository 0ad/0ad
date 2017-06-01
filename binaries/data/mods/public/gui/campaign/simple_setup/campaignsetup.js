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

	Engine.GetGUIObjectByName("CampaignImage").sprite = "cropped:" + 400/512 + "," + 300/512 + ":session/icons/mappreview/nopreview.png";

	GenerateCampaignList();
}

function GenerateCampaignList()
{
	let selection = Engine.GetGUIObjectByName("campaignSelection");
	if (selection.selected !== -1)
	 displayCampaignDetails();

	let list = [];
	for (let key in g_CampaignsAvailable)
		list.push({ "directories" : key, "name" : g_CampaignsAvailable[key].Name });

	// change array of object into object of array.
	list = prepareForDropdown(list);

	// Push to GUI
	selection.selected = -1;
	selection.list_name = list.name || [];

	// Change these last, otherwise crash
	// TODO: do we need both of those? I'm unsure.
	selection.list = list.directories || [];
	selection.list_data = list.directories || [];

//	replaySelection.selected = replaySelection.list.findIndex(directory => directory == g_SelectedReplayDirectory);

//	displayReplayDetails();
}

function displayCampaignDetails()
{
	let selection = Engine.GetGUIObjectByName("campaignSelection");
	if (selection.selected === -1)
		return;

	g_SelectedCampaign = selection.list[selection.selected];

	Engine.GetGUIObjectByName("startCampButton").enabled = true;
	Engine.GetGUIObjectByName("campaignOptionsButton").enabled = true;

	Engine.GetGUIObjectByName("CampaignTitle").caption = translate(g_CampaignsAvailable[g_SelectedCampaign].Name);	
	Engine.GetGUIObjectByName("campaignDesc").caption = translate(g_CampaignsAvailable[g_SelectedCampaign].Description);	

	if (g_CampaignsAvailable[g_SelectedCampaign].Image)
		Engine.GetGUIObjectByName("CampaignImage").sprite = "stretched:" + g_CampaignsAvailable[g_SelectedCampaign].Image;
	else
		Engine.GetGUIObjectByName("CampaignImage").sprite = "cropped:" + 400/512 + "," + 300/512 + ":session/icons/mappreview/nopreview.png";

}

function startCampaign()
{
	Engine.PushGuiPage("page_newcampaign.xml", { "campaignID" : g_SelectedCampaign, "campaignData" : g_CampaignsAvailable[g_SelectedCampaign] });
}
