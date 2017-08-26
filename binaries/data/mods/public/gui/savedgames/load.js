var g_SavedGamesMetadata = [];

/**
 * Needed for formatPlayerInfo to show the player civs in the details.
 */
const g_CivData = loadCivData(false, false);

function init()
{
	let savedGames = Engine.GetSavedGames();

	// Get current game version and loaded mods
	let engineInfo = Engine.GetEngineInfo();

	if (Engine.GetGUIObjectByName("compatibilityFilter").checked)
		savedGames = savedGames.filter(game => isCompatibleSavegame(game.metadata, engineInfo));

	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	gameSelection.enabled = !!savedGames.length;
	Engine.GetGUIObjectByName("gameSelectionFeedback").hidden = !!savedGames.length;

	let selectedGameId = gameSelection.list_data[gameSelection.selected];

	// Save metadata for the detailed view
	g_SavedGamesMetadata = savedGames.map(game =>
	{
		game.metadata.id = game.id;
		return game.metadata;
	});

	const sortKey = gameSelection.selected_column;
	const sortOrder = gameSelection.selected_column_order;
	g_SavedGamesMetadata = g_SavedGamesMetadata.sort((a, b) =>
	{
		let cmpA, cmpB;
		switch (sortKey)
		{
		case 'date':
			cmpA = +a.time;
			cmpB = +b.time;
			break;
		case 'mapName':
			cmpA = translate(a.initAttributes.settings.Name);
			cmpB = translate(b.initAttributes.settings.Name);
			break;
		case 'mapType':
			cmpA = translateMapType(a.initAttributes.mapType);
			cmpB = translateMapType(b.initAttributes.mapType);
			break;
		case 'description':
			cmpA = a.description;
			cmpB = b.description;
			break;
		}

		if (cmpA < cmpB)
			return -sortOrder;
		else if (cmpA > cmpB)
			return +sortOrder;

		return 0;
	});

	let list = g_SavedGamesMetadata.map(metadata => {
		let isCompatible = isCompatibleSavegame(metadata, engineInfo);
		return {
			"date": generateSavegameDateString(metadata, engineInfo),
			"mapName": compatibilityColor(translate(metadata.initAttributes.settings.Name), isCompatible),
			"mapType": compatibilityColor(translateMapType(metadata.initAttributes.mapType), isCompatible),
			"description": compatibilityColor(metadata.description, isCompatible)
		};
	});

	if (list.length)
		list = prepareForDropdown(list);

	gameSelection.list_date = list.date || [];
	gameSelection.list_mapName = list.mapName || [];
	gameSelection.list_mapType = list.mapType || [];
	gameSelection.list_description = list.description || [];

	// Change these last, otherwise crash
	// list strings used in the delete dialog
	gameSelection.list = g_SavedGamesMetadata.map(metadata => generateSavegameLabel(metadata, engineInfo));
	gameSelection.list_data = g_SavedGamesMetadata.map(metadata => metadata.id);

	let selectedGameIndex = g_SavedGamesMetadata.findIndex(metadata => metadata.id == selectedGameId);
	if (selectedGameIndex != -1)
		gameSelection.selected = selectedGameIndex;
	else if (gameSelection.selected >= g_SavedGamesMetadata.length) // happens when deleting the last saved game
		gameSelection.selected = g_SavedGamesMetadata.length - 1;
	else if (gameSelection.selected == -1 && g_SavedGamesMetadata.length)
		gameSelection.selected = 0;

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
