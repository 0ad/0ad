var g_SelectedLevel = null;

function init(data)
{
	if (!data)
	{
		warn("Loading campaign menu without a campaign loaded")
		return false;
	}

	g_CampaignID = data.ID;
	g_CampaignTemplate = data.template;
	g_CampaignSave = data.save;
	g_CampaignData = data.data;

	generateLevelList();
}

function generateLevelList()
{
	// TODO: remember old selection?
	let selection = Engine.GetGUIObjectByName("levelSelection");

	let list = [];
	for (let key in g_CampaignTemplate.Levels)
	{
		let level = g_CampaignTemplate.Levels[key];

		if (!("ShowUnavailable" in g_CampaignTemplate) && !hasRequirements(level))
			continue;

		let status = hasRequirements(level) ? "available" : "unavailable";

		list.push({ "ID" : key, "name" : level.Name, "status" : status });
	}
	list.sort((a, b) => g_CampaignTemplate.Order.indexOf(a.ID) - g_CampaignTemplate.Order.indexOf(b.ID));

	// change array of object into object of array.
	list = prepareForDropdown(list);

	// Push to GUI
	selection.selected = -1;
	selection.list_name = list.name || [];
	selection.list_status = list.status || [];

	// Change these last, otherwise crash
	// TODO: do we need both of those? I'm unsure.
	selection.list = list.ID || [];
	selection.list_data = list.ID || [];

//	replaySelection.selected = replaySelection.list.findIndex(directory => directory == g_SelectedReplayDirectory);

//	displayReplayDetails();

}

function displayLevelDetails()
{
	// TODO: actually update description on the right-hand side
	let selection = Engine.GetGUIObjectByName("levelSelection");

	if (selection.selected === -1)
		return;

	let level = g_CampaignTemplate.Levels[selection.list[selection.selected]];

	if (!hasRequirements(level))
	{
		Engine.GetGUIObjectByName("startCampButton").enabled = false;
		return;
	}

	Engine.GetGUIObjectByName("startCampButton").enabled = true;
	g_SelectedLevel = level;
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
	Engine.SwitchGuiPage("page_pregame.xml", {});
}
