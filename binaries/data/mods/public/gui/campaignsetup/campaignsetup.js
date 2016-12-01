/**
 * Used for checking replay compatibility.
 */
const g_EngineInfo = Engine.GetEngineInfo();

var g_CampaignsAvailable = []; // folder names

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
		if (mods[k].type && mods[k].type != "campaign")
			return;
		g_CampaignsAvailable[k] = mods[k];
	});

	let modsEnabled = getExistingModsFromConfig();

	if (Object.keys(g_CampaignsAvailable).filter(function(i) { return modsEnabled.indexOf(i) !== -1; }))
		warn("Warning: a campaign mod is already loaded. Loading another one is undefined behavior.");

	GenerateCampaignList();
}

function GenerateCampaignList()
{
	// Remember previously selected
	let oldSelection = Engine.GetGUIObjectByName("campaignSelection");
	//if (oldSelection.selected != -1)
	//	g_SelectedReplayDirectory = g_ReplaysFiltered[replaySelection.selected].directory;

	warn(uneval(g_CampaignsAvailable));
/*
	let list = g_CampaignsAvailable.map(camp => {
		return {
			"directories": replay.directory,
			"months": greyout(getReplayDateTime(replay), works),
			"popCaps": greyout(translatePopulationCapacity(replay.attribs.settings.PopulationCap), works),
			"mapNames": greyout(getReplayMapName(replay), works),
			"mapSizes": greyout(translateMapSize(replay.attribs.settings.Size), works),
			"durations": greyout(getReplayDuration(replay), works),
			"playerNames": greyout(getReplayPlayernames(replay), works)
		};
	});*/
}