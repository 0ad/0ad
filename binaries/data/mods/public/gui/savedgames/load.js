var g_SavedGamesMetadata = [];

/**
 * Needed for formatPlayerInfo to show the player civs in the details.
 */
const g_CivData = loadCivData();

function init()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let savedGames = Engine.GetSavedGames().sort(sortDecreasingDate);
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

	gameSelection.list = savedGames.map(game => generateLabel(game.metadata, engineInfo));
	gameSelection.list_data = savedGames.map(game => game.id);

	if (gameSelection.selected == -1)
		gameSelection.selected = 0;
	else if (gameSelection.selected >= savedGames.length) // happens when deleting the last saved game
		gameSelection.selected = savedGames.length - 1;
	else
		selectionChanged();

	Engine.GetGUIObjectByName("deleteGameButton").tooltip = deleteTooltip();
}

function selectionChanged()
{
	let metadata = g_SavedGamesMetadata[Engine.GetGUIObjectByName("gameSelection").selected];
	Engine.GetGUIObjectByName("invalidGame").hidden = !!metadata;
	Engine.GetGUIObjectByName("validGame").hidden = !metadata;
	Engine.GetGUIObjectByName("loadGameButton").enabled = !!metadata;
	Engine.GetGUIObjectByName("deleteGameButton").enabled = !!metadata;

	if (!metadata)
		return;

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
}

function loadGame()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let gameId = gameSelection.list_data[gameSelection.selected];
	let metadata = g_SavedGamesMetadata[gameSelection.selected];

	// Check compatibility before really loading it
	let engineInfo = Engine.GetEngineInfo();
	let sameMods = hasSameMods(metadata, engineInfo);
	let sameEngineVersion = hasSameEngineVersion(metadata, engineInfo);
	let sameSavegameVersion = hasSameSavegameVersion(metadata, engineInfo);

	if (sameEngineVersion && sameSavegameVersion && sameMods)
	{
		reallyLoadGame(gameId);
		return;
	}

	// Version not compatible ... ask for confirmation
	let message = translate("This saved game may not be compatible:");

	if (!sameEngineVersion)
	{
		if (metadata.engine_version)
			message += "\n" + sprintf(translate("It needs 0 A.D. version %(requiredVersion)s, while you are running version %(currentVersion)s."), {
				"requiredVersion": metadata.engine_version,
				"currentVersion": engineInfo.engine_version
			});
		else
			message += "\n" + translate("It needs an older version of 0 A.D.");
	}

	if (!sameSavegameVersion)
		message += "\n" + sprintf(translate("It needs 0 A.D. savegame version %(requiredVersion)s, while you have savegame version %(currentVersion)s."), {
			"requiredVersion": metadata.version_major,
			"currentVersion": engineInfo.version_major
		});

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
	);
}

function reallyLoadGame(gameId)
{
	let metadata = Engine.StartSavedGame(gameId);
	if (!metadata)
	{
		// Probably the file wasn't found
		// Show error and refresh saved game list
		error("Could not load saved game: " + gameId);
		init();
		return;
	}

	let pData = metadata.initAttributes.settings.PlayerData[metadata.playerID];

	Engine.SwitchGuiPage("page_loading.xml", {
		"attribs": metadata.initAttributes,
		"isNetworked" : false,
		"playerAssignments": {
			"local": {
				"name": pData ? pData.Name : singleplayerName(),
				"player": metadata.playerID
			}
		},
		"savedGUIData": metadata.gui
	});
}
