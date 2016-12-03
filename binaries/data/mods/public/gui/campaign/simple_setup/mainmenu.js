var g_SelectedScenario = null;

function init(data)
{
	// from public mod campaignsetup/campaign_utils.js, this loads the campaign file and sets g_CurrentCampaign and g_CampaignData
	loadCurrentCampaignState();

	generateScenarioList();
}

function generateScenarioList()
{
	// TODO: remember old selection?
	let selection = Engine.GetGUIObjectByName("scenarioSelection");

	let list = [];
	for (let key in g_CampaignTemplate.Campaign.Scenarios)
	{
		let scenario = g_CampaignTemplate.Campaign.Scenarios[key];

		if (!("ShowUnavailable" in g_CampaignTemplate.Campaign) && !hasRequirements(scenario))
			continue;

		let status = hasRequirements(scenario) ? "available" : "unavailable";

		list.push({ "xmlName" : key, "RID" : scenario.ReadableID, "name" : scenario.Name, "order" : scenario.ID, "status" : status });
	}

	list.sort((a, b) => a.order - b.order);

	// change array of object into object of array.
	list = prepareForDropdown(list);

	// Push to GUI
	selection.selected = -1;
	selection.list_ID = list.RID || [];
	selection.list_name = list.name || [];
	selection.list_status = list.status || [];

	// Change these last, otherwise crash
	// TODO: do we need both of those? I'm unsure.
	selection.list = list.xmlName || [];
	selection.list_data = list.xmlName || [];

//	replaySelection.selected = replaySelection.list.findIndex(directory => directory == g_SelectedReplayDirectory);

//	displayReplayDetails();

}

function displayScenarioDetails()
{
	// TODO: actually update description on the right-hand side
	let selection = Engine.GetGUIObjectByName("scenarioSelection");

	if (selection.selected === -1)
		return;

	let scenario = g_CampaignTemplate.Campaign.Scenarios[selection.list[selection.selected]];

	if (!hasRequirements(scenario))
	{
		Engine.GetGUIObjectByName("startCampButton").enabled = false;
		return;
	}

	Engine.GetGUIObjectByName("startCampButton").enabled = true;
	g_SelectedScenario = scenario;
}