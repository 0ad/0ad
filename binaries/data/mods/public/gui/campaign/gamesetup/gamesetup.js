// Here should go functions to set up Campaign games.
var g_GameAttributes = { "settings": {} };

var g_DefaultPlayerData = [];

function sanitizePlayerData(playerData)
{
	// Remove gaia
	if (playerData.length && !playerData[0])
		playerData.shift();

	playerData.forEach((pData, index) => {
		pData.Color = pData.Color;
		pData.Civ = pData.Civ;

		// Use default AI if the map doesn't specify any explicitly
		if (!("AI" in pData))
			pData.AI = g_DefaultPlayerData[index].AI;

		if (!("AIDiff" in pData))
			pData.AIDiff = g_DefaultPlayerData[index].AIDiff;
	});
}


// TODO: this is a minimalist patchwork from gamesetup.Js and only barely attempts to work.

function launchGame(level, levelID)
{
	if (!level.Map)
	{
		warn("cannot start scenario: no maps specified.");
		return;
	}

	loadSettingsValues();

	g_DefaultPlayerData = g_Settings.PlayerDefaults;
	g_DefaultPlayerData.shift();

	let mapData = Engine.LoadMapSettings("maps/" + level.Map);
	if (!mapData)
	{
		warn("Could not load map");
		return;
	}

	let mapSettings = mapData && mapData.settings ? deepcopy(mapData.settings) : {};

	if (mapSettings.PlayerData)
		sanitizePlayerData(mapSettings.PlayerData);

	// Copy any new settings
	g_GameAttributes.map = "maps/" + level.Map;
	g_GameAttributes.script = mapSettings.Script;

	for (let prop in mapSettings)
		g_GameAttributes.settings[prop] = mapSettings[prop];

	// TODO: support default victory conditions?
	g_GameAttributes.settings.TriggerScripts = g_GameAttributes.settings.TriggerScripts || [];

	g_GameAttributes.settings.mapType = "scenario";
	g_GameAttributes.mapType = "scenario";

	// Seed used for both map generation and simulation
	g_GameAttributes.settings.Seed = Math.floor(Math.random() * Math.pow(2, 32));
	g_GameAttributes.settings.AISeed = Math.floor(Math.random() * Math.pow(2, 32));

	g_GameAttributes.matchID = Engine.GetMatchID();

	// TODO: player should be defined in the map or the campaign at the least.
	let playerID = 1;

	g_GameAttributes.campaignData = {"ID" : g_CampaignID, "template" : g_CampaignTemplate, "save": g_CampaignSave, "data" : g_CampaignData, "level" : levelID};

	Engine.StartGame(g_GameAttributes, playerID);
	Engine.SwitchGuiPage("page_loading.xml", {
		"attribs": g_GameAttributes,
		"isNetworked" : false,
		"playerAssignments": {}
	});
	return g_GameAttributes.matchID;
}
