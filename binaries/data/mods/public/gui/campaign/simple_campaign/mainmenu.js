var g_SelectedLevel = null;

var g_SavedGamesMetadata = [];

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
	selectionChanged();

	Engine.GetGUIObjectByName("mapPreview").sprite = "cropped:" + 400/512 + "," + 300/512 + ":session/icons/mappreview/nopreview.png";

	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let savedGames = Engine.GetSavedGames().sort(sortDecreasingDate).filter(game => game.metadata.initAttributes && game.metadata.initAttributes.campaignData && game.metadata.initAttributes.campaignData.ID == g_CampaignID);
	gameSelection.enabled = !!savedGames.length;
	if (!savedGames.length)
	{
		gameSelection.list = [translate("No saved games found")];
		gameSelection.selected = -1;
		return;
	}

	// Get current game version and loaded mods
	let engineInfo = Engine.GetEngineInfo();

	g_SavedGamesMetadata = savedGames.map(game => game.metadata);

	gameSelection.list = savedGames.map(game => generateCampaignLabel(game.metadata, engineInfo));
	gameSelection.list_data = savedGames.map(game => game.id);

	saveSelectionChanged();
}

function generateLevelList()
{
	// TODO: remember old selection?
	let selection = Engine.GetGUIObjectByName("levelSelection");

	let list = [];
	for (let key in g_CampaignTemplate.Levels)
	{
		let level = g_CampaignTemplate.Levels[key];

		if (!("ShowUnavailable" in g_CampaignTemplate) || !g_CampaignTemplate.ShowUnavailable && !hasRequirements(level))
			continue;

		let status = "";
		let name = level.Name;
		if (!hasRequirements(level))
		{
			status = "not unlocked yet";
			name = "[color=\"gray\"]" + name + "[/color]";
		}
		list.push({ "ID" : key, "name" : name, "status" : status });
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

function displayLevelDetails(levelID)
{
	let level = g_CampaignTemplate.Levels[levelID];

	// TODO: load from map file if not present
	Engine.GetGUIObjectByName("scenarioName").caption = translate(level.Name);
	Engine.GetGUIObjectByName("scenarioDesc").caption = translate(level.Description);

	// todo: ibidem
	if (level.Preview)
		Engine.GetGUIObjectByName("mapPreview").sprite = "cropped:" + 400/512 + "," + 300/512 + ":" + level.Preview;
	else
		Engine.GetGUIObjectByName("mapPreview").sprite = "cropped:" + 400/512 + "," + 300/512 + ":session/icons/mappreview/nopreview.png";

	g_SelectedLevel = levelID;

	if (!hasRequirements(level))
	{
		Engine.GetGUIObjectByName("startButton").enabled = false;
		return;
	}

	Engine.GetGUIObjectByName("startButton").enabled = true;
}

function selectionChanged()
{
	let selection = Engine.GetGUIObjectByName("levelSelection");

	if (selection.selected === -1)
	{
		Engine.GetGUIObjectByName("startButton").enabled = false;
		Engine.GetGUIObjectByName("startButton").hidden = false;
		Engine.GetGUIObjectByName("loadSavedButton").hidden = true;
		return;
	}

	Engine.GetGUIObjectByName("loadSavedButton").hidden = true;
	Engine.GetGUIObjectByName("startButton").hidden = false;

	let selec = Engine.GetGUIObjectByName("gameSelection");
	selec.selected = -1;

	displayLevelDetails(selection.list[selection.selected]);
}

function saveSelectionChanged()
{
	let metadata = g_SavedGamesMetadata[Engine.GetGUIObjectByName("gameSelection").selected];

	if (!metadata)
		return;

	// fetch campaign scenario metadata if present.
	let scenarioID = metadata.initAttributes.campaignData.level;

	let selection = Engine.GetGUIObjectByName("levelSelection");
	selection.selected = -1;
	for (let level of selection.list_data)
		if (level == scenarioID)
		{
			displayLevelDetails(level);
			break;
		}

	Engine.GetGUIObjectByName("loadSavedButton").hidden = false;
	Engine.GetGUIObjectByName("startButton").hidden = true;
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
	);*/
}

function generateCampaignLabel(metadata, engineInfo)
{
	let dateTimeString = Engine.FormatMillisecondsIntoDateString(metadata.time*1000, translate("yyyy-MM-dd HH:mm:ss"));
	let dateString = sprintf(translate("\\[%(date)s]"), { "date": dateTimeString });

	if (engineInfo)
	{
		if (!hasSameSavegameVersion(metadata, engineInfo) || !hasSameEngineVersion(metadata, engineInfo))
			dateString = "[color=\"red\"]" + dateString + "[/color]";
		else if (!hasSameMods(metadata, engineInfo))
			dateString = "[color=\"orange\"]" + dateString + "[/color]";
	}

	return sprintf(
		metadata.description ?
			translate("%(dateString)s %(level)s - %(description)s") :
			translate("%(dateString)s %(level)s"),
		{
			"dateString": dateString,
			"level": metadata.initAttributes.campaignData.template.Levels[metadata.initAttributes.campaignData.level].Name,
			"description": metadata.description || ""
		}
	);
}

function exitCampaignMode(exitGame = false)
{
	// TODO: should this be here?
	saveCurrentCampaign();

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
