var g_Campaigns = [];

var g_CampaignTemplate = null;

function init()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");

	let campaigns = Engine.BuildDirEntList("campaignsaves/", "*.0adcampaign", false);

	if (!campaigns.length)
	{
		gameSelection.list = [translate("No ongoing campaigns found")];
		gameSelection.selected = -1;
		selectionChanged();
		Engine.GetGUIObjectByName("loadGameButton").enabled = false;
		Engine.GetGUIObjectByName("deleteGameButton").enabled = false;
		return;
	}

	gameSelection.list = campaigns.map(path => generateLabel(pathToGame(path)));
	gameSelection.list_data = campaigns.map(path => pathToGame(path));

	if (gameSelection.selected == -1)
		gameSelection.selected = 0;
	else if (gameSelection.selected >= campaigns.length) // happens when deleting the last saved game
		gameSelection.selected = campaigns.length - 1;
	else
		selectionChanged();
}

function pathToGame(path)
{
	return path.replace("campaignsaves/","").replace(".0adcampaign","");
}

function generateLabel(game)
{
	let campaignData = Engine.LoadCampaign(game);
	if (!campaignData)
		return "Incompatible - " + game;

	if (!loadCampaignTemplate(campaignData.campaign_state.campaign))
		return "Incompatible - " + game;

	return game + " - " + g_CampaignTemplate.Name;
}

function selectionChanged()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let selectionEmpty = gameSelection.selected == -1;
	Engine.GetGUIObjectByName("invalidGame").hidden = !selectionEmpty;
	Engine.GetGUIObjectByName("validGame").hidden = selectionEmpty;

	if (selectionEmpty)
		return;

/*
	Engine.GetGUIObjectByName("savedMapName").caption = translate(metadata.initAttributes.settings.Name);
	let mapData = getMapDescriptionAndPreview(metadata.initAttributes.mapType, metadata.initAttributes.map);
	setMapPreviewImage("savedInfoPreview", mapData.preview);

	Engine.GetGUIObjectByName("savedPlayers").caption = metadata.initAttributes.settings.PlayerData.length - 1;
	Engine.GetGUIObjectByName("savedPlayedTime").caption = timeToString(metadata.gui.timeElapsed ? metadata.gui.timeElapsed : 0);
	Engine.GetGUIObjectByName("savedMapType").caption = translateMapType(metadata.initAttributes.mapType);
	Engine.GetGUIObjectByName("savedMapSize").caption = translateMapSize(metadata.initAttributes.settings.Size);
	Engine.GetGUIObjectByName("savedVictory").caption = translateVictoryCondition(metadata.initAttributes.settings.GameType);

	let caption = sprintf(translate("Mods: %(mods)s"), { "mods": metadata.mods.join(translate(", ")) });
	if (!hasSameMods(metadata, Engine.GetEngineInfo()))
		caption = "[color=\"orange\"]" + caption + "[/color]";
	Engine.GetGUIObjectByName("savedMods").caption = caption;

	Engine.GetGUIObjectByName("savedPlayersNames").caption = formatPlayerInfo(
		metadata.initAttributes.settings.PlayerData,
		metadata.gui.states
	);
	*/
}

function loadGame()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
/*
	// Check compatibility before really loading it
	let sameMods = hasSameMods(metadata, engineInfo);

	if (sameEngineVersion && sameSavegameVersion && sameMods)
	{
		reallyLoadGame(gameId);
		return;
	}

	if (!sameMods)
	{
		if (!metadata.mods)
			metadata.mods = [];

		message += translate("The savegame needs a different set of mods:") + "\n" +
			sprintf(translate("Required: %(mods)s"), {
				"mods": metadata.mods.join(translate(", "))
			}) + "\n" +
			sprintf(translate("Active: %(mods)s"), {
				"mods": engineInfo.mods.join(translate(", "))
			});
	}

	message += "\n" + translate("Do you still want to proceed?");

	messageBox(
		500, 250,
		message,
		translate("Warning"),
		[translate("No"), translate("Yes")],
		[init, function(){ reallyLoadGame(gameId); }]
	);*/
}

function reallyLoadGame(gameId)
{
	
}

function deleteCampaign()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let campaign = gameSelection.list_data[gameSelection.selected];

	if (!campaign)
		return;

	messageBox(
		500, 200,
		sprintf(translate("\"%(label)s\""), {
			"label": gameSelection.list[gameSelection.selected]
		}) + "\n" + translate("Campaign will be permanently deleted, are you sure?"),
		translate("DELETE"),
		[translate("No"), translate("Yes")],
		[null, function(){ reallyDeleteCampaign(campaign); }]
	);
}

function reallyDeleteCampaign(name)
{
	if (!Engine.DeleteCampaignGame(name))
		error("Could not delete campaign game " + name);

	// re-run init to refresh.
	init();
}
