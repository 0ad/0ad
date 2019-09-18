var g_SavegameWriter;

var g_SavedGamesMetadata = [];

/**
 * Needed for formatPlayerInfo to show the player civs in the details.
 */
const g_CivData = loadCivData(false, false);

function init(data)
{
	let save = Engine.IsGameStarted();
	if (save)
		g_SavegameWriter = new SavegameWriter(data);

	let confirmButton = Engine.GetGUIObjectByName("confirmButton");
	confirmButton.caption = save ? translate("Save") : translate("Load");
	confirmButton.onPress = save ? () => { g_SavegameWriter.saveGame(); } : loadGame;
	Engine.GetGUIObjectByName("title").caption = save ? translate("Save Game") : translate("Load Game")
	Engine.GetGUIObjectByName("saveGameDesc").hidden = !save;

	updateSavegameList();

	// Select the most recent savegame to be loaded, or no savegame to be overwritten
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	if (!save && gameSelection.list.length)
		gameSelection.selected = 0;
	else
		selectionChanged();
}

function updateSavegameList()
{
	let savedGames = Engine.GetSavedGames();

	// Get current game version and loaded mods
	let engineInfo = Engine.GetEngineInfo();

	if (Engine.GetGUIObjectByName("compatibilityFilter").checked)
		savedGames = savedGames.filter(game => isCompatibleSavegame(game.metadata, engineInfo));

	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	gameSelection.enabled = !!savedGames.length;
	gameSelection.onSelectionChange = selectionChanged;
	gameSelection.onSelectionColumnChange = updateSavegameList;
	gameSelection.onMouseLeftDoubleClickItem = loadGame;

	Engine.GetGUIObjectByName("gameSelectionFeedback").hidden = !!savedGames.length;

	let selectedGameId = gameSelection.list_data[gameSelection.selected];

	// Save metadata for the detailed view
	g_SavedGamesMetadata = savedGames.map(game => {
		game.metadata.id = game.id;
		return game.metadata;
	});

	let sortKey = gameSelection.selected_column;
	let sortOrder = gameSelection.selected_column_order;
	g_SavedGamesMetadata = g_SavedGamesMetadata.sort((a, b) => {
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

	Engine.GetGUIObjectByName("deleteGameButton").tooltip = deleteTooltip();
}

function selectionChanged()
{
	let metadata = g_SavedGamesMetadata[Engine.GetGUIObjectByName("gameSelection").selected];
	Engine.GetGUIObjectByName("invalidGame").hidden = !!metadata;
	Engine.GetGUIObjectByName("validGame").hidden = !metadata;
	Engine.GetGUIObjectByName("confirmButton").enabled = !!metadata || Engine.IsGameStarted();
	Engine.GetGUIObjectByName("deleteGameButton").enabled = !!metadata;

	if (!metadata)
		return;

	Engine.GetGUIObjectByName("savedMapName").caption = translate(metadata.initAttributes.settings.Name);
	Engine.GetGUIObjectByName("savedInfoPreview").sprite = getMapPreviewImage(
		getMapDescriptionAndPreview(metadata.initAttributes.mapType, metadata.initAttributes.map, metadata.initAttributes).preview);
	Engine.GetGUIObjectByName("savedPlayers").caption = metadata.initAttributes.settings.PlayerData.length - 1;
	Engine.GetGUIObjectByName("savedPlayedTime").caption = timeToString(metadata.gui.timeElapsed ? metadata.gui.timeElapsed : 0);
	Engine.GetGUIObjectByName("savedMapType").caption = translateMapType(metadata.initAttributes.mapType);
	Engine.GetGUIObjectByName("savedMapSize").caption = translateMapSize(metadata.initAttributes.settings.Size);
	Engine.GetGUIObjectByName("savedVictory").caption = metadata.initAttributes.settings.VictoryConditions.map(victoryConditionName => translateVictoryCondition(victoryConditionName)).join(translate(", "));

	let caption = sprintf(translate("Mods: %(mods)s"), { "mods": modsToString(metadata.mods) });
	if (!hasSameMods(metadata.mods, Engine.GetEngineInfo().mods))
		caption = coloredText(caption, "orange");
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
	let sameMods = hasSameMods(metadata.mods, engineInfo.mods);
	let sameEngineVersion = hasSameEngineVersion(metadata, engineInfo);

	if (sameEngineVersion && sameMods)
	{
		reallyLoadGame(gameId);
		return;
	}

	// Version not compatible ... ask for confirmation
	let message = "";

	if (!sameEngineVersion)
		if (metadata.engine_version)
			message += sprintf(translate("This savegame needs 0 A.D. version %(requiredVersion)s, while you are running version %(currentVersion)s."), {
				"requiredVersion": metadata.engine_version,
				"currentVersion": engineInfo.engine_version
			}) + "\n";
		else
			message += translate("This savegame needs an older version of 0 A.D.") + "\n";

	if (!sameMods)
	{
		if (!metadata.mods)
			metadata.mods = [];

		message += translate("This savegame needs a different sequence of mods:") + "\n" +
			comparedModsString(metadata.mods, engineInfo.mods) + "\n";
	}

	message += translate("Do you still want to proceed?");

	messageBox(
		500, 250,
		message,
		translate("Warning"),
		[translate("No"), translate("Yes")],
		[undefined, () => { reallyLoadGame(gameId); }]);
}

function reallyLoadGame(gameId)
{
	let metadata = Engine.StartSavedGame(gameId);
	if (!metadata)
	{
		error("Could not load saved game: " + gameId);
		return;
	}

	let pData = metadata.initAttributes.settings.PlayerData[metadata.playerID];

	Engine.SwitchGuiPage("page_loading.xml", {
		"attribs": metadata.initAttributes,
		"playerAssignments": {
			"local": {
				"name": pData ? pData.Name : singleplayerName(),
				"player": metadata.playerID
			}
		},
		"savedGUIData": metadata.gui
	});
}
