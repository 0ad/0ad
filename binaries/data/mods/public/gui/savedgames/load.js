var g_SavedGamesMetadata = [];

function init()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");

	var savedGames = Engine.GetSavedGames().sort(sortDecreasingDate);
	if (!savedGames.length)
	{
		gameSelection.list = [translate("No saved games found")];
		gameSelection.selected = 0;
		Engine.GetGUIObjectByName("loadGameButton").enabled = false;
		Engine.GetGUIObjectByName("deleteGameButton").enabled = false;
		return;
	}

	// Get current game version and loaded mods
	var engineInfo = Engine.GetEngineInfo();

	g_SavedGamesMetadata = savedGames.map(game => game.metadata);

	gameSelection.list = savedGames.map(game => generateLabel(game.metadata, engineInfo));
	gameSelection.list_data = savedGames.map(game => game.id);

	if (gameSelection.selected == -1)
		gameSelection.selected = 0;
	else if (gameSelection.selected >= savedGames.length) // happens when deleting the last saved game
		gameSelection.selected = savedGames.length - 1;
}

function loadGame()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");
	var gameId = gameSelection.list_data[gameSelection.selected];
	var gameLabel = gameSelection.list[gameSelection.selected];
	var metadata = g_SavedGamesMetadata[gameSelection.selected];

	// Check compatibility before really loading it
	var engineInfo = Engine.GetEngineInfo();
	var sameMods = hasSameMods(metadata, engineInfo);
	var sameEngineVersion = hasSameEngineVersion(metadata, engineInfo);
	var sameSavegameVersion = hasSameSavegameVersion(metadata, engineInfo);

	if (sameEngineVersion && sameSavegameVersion && sameMods)
	{
		reallyLoadGame(gameId);
		return;
	}

	// Version not compatible ... ask for confirmation
	var message = translate("This saved game may not be compatible:");

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

		message += translate("The savegame needs a different set of mods:") + "\n";
		errMsg += sprintf(translate("Required: %(mods)s"), { "mods": metadata.mods.join(translate(", ")) }) + "\n";
		errMsg += sprintf(translate("Active: %(mods)s"), { "mods": engineInfo.mods.join(translate(", ")) });
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
	var metadata = Engine.StartSavedGame(gameId);
	if (!metadata)
	{
		// Probably the file wasn't found
		// Show error and refresh saved game list
		error("Could not load saved game: " + gameId);
		init();
		return;
	}

	Engine.SwitchGuiPage("page_loading.xml", {
		"attribs": metadata.initAttributes,
		"isNetworked" : false,
		"playerAssignments": {
			"local": {
				"name": metadata.initAttributes.settings.PlayerData[metadata.playerID].Name,
				"player": metadata.playerID
			}
		},
		"savedGUIData": metadata.gui
	});
}

function deleteGame()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");
	var gameLabel = gameSelection.list[gameSelection.selected];
	var gameID = gameSelection.list_data[gameSelection.selected];

	// Ask for confirmation
	messageBox(
		500, 200,
		sprintf(translate("\"%(label)s\""), { "label": gameLabel }) + "\n" +
			translate("Saved game will be permanently deleted, are you sure?"),
		translate("DELETE"),
		[translate("No"), translate("Yes")],
		[null, function(){ reallyDeleteGame(gameID); }]
	);
}

function deleteGameWithoutConfirmation()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");
	var gameID = gameSelection.list_data[gameSelection.selected];
	reallyDeleteGame(gameID);
}

function reallyDeleteGame(gameID)
{
	if (!Engine.DeleteSavedGame(gameID))
		error("Could not delete saved game: " + gameID);

	// Run init again to refresh saved game list
	init();
}
