/**
 * Used for checking replay compatibility.
 */
const g_EngineInfo = Engine.GetEngineInfo();

var g_CampaignsAvailable = {}; // folder names

/*
 * Initializes the campaign window.
 * Loads all compatible campaign mods
 * Allows you to start them.
 */

// TODO: make sure we don't load two campaign mods
// TODO: tell the campaign mod its own name

function init(data)
{
	LoadCampaignMods(data);
}

function LoadCampaignMods(data)
{
	let mods = Engine.GetAvailableMods();
	let keys = ["name", "label", "description", "dependencies", "version"];
	Object.keys(mods).forEach(function(k) {
		for (let i = 0; i < keys.length; ++i)
			if (!keys[i] in mods[k])
			{
				log("Skipping mod '"+k+"'. Missing property '"+keys[i]+"'.");
				return;
			}
		// skip non-campaign mods
		if (!mods[k].type || mods[k].type != "campaign")
			return;
		g_CampaignsAvailable[k] = mods[k];
	});

	let modsEnabled = getExistingModsFromConfig();

	warn ("mods enabled " + uneval(modsEnabled))
	warn ("mods available " + uneval(g_CampaignsAvailable))

	if (Object.keys(g_CampaignsAvailable).some(foldername => modsEnabled.indexOf(foldername) !== -1))
		warn("Warning: a campaign mod is already loaded. Loading another one is undefined behavior.");

	GenerateCampaignList();
}

function GenerateCampaignList()
{
	// Remember previously selected
	let selection = Engine.GetGUIObjectByName("campaignSelection");
	//if (oldSelection.selected != -1)
	//	g_SelectedReplayDirectory = g_ReplaysFiltered[replaySelection.selected].directory;

	warn(uneval(g_CampaignsAvailable));

	let list = [];
	for (let key in g_CampaignsAvailable)
		list.push({ "directories" : key, "name" : g_CampaignsAvailable[key].name, "difficulty" : "TODO" });

	// change array of object into object of array.
	list = prepareForDropdown(list);

	// Push to GUI
	selection.selected = -1;
	selection.list_name = list.name || [];
	selection.list_difficulty = list.difficulty || [];

	// Change these last, otherwise crash
	selection.list = list.directories || [];
	selection.list_data = list.directories || [];

	warn(uneval(list))
//	replaySelection.selected = replaySelection.list.findIndex(directory => directory == g_SelectedReplayDirectory);

//	displayReplayDetails();
}