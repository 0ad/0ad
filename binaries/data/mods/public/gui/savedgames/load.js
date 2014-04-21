var gameMetadatas = [];

function init()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");

	var savedGames = Engine.GetSavedGames();
	if (savedGames.length == 0)
	{
		gameSelection.list = [translate("No saved games found")];
		gameSelection.selected = 0;
		Engine.GetGUIObjectByName("loadGameButton").enabled = false;
		Engine.GetGUIObjectByName("deleteGameButton").enabled = false;
		return;
	}

	savedGames.sort(sortDecreasingDate);

	// get current game version and loaded mods 
	var engineInfo = Engine.GetEngineInfo();

	var gameListIds = [ game.id for each (game in savedGames) ];
	var gameListLabels = [ generateLabel(game.metadata, engineInfo) for each (game in savedGames) ];
	gameMetadatas = [ game.metadata for each (game in savedGames) ];

	gameSelection.list = gameListLabels;
	gameSelection.list_data = gameListIds;
	if (gameSelection.selected == -1) 
		gameSelection.selected = 0;
}

function loadGame()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");
	var gameId = gameSelection.list_data[gameSelection.selected];
	var gameLabel = gameSelection.list[gameSelection.selected];
	var metadata = gameMetadatas[gameSelection.selected];

	// check game compatibility before really loading it
	var engineInfo = Engine.GetEngineInfo();
	if (!hasSameVersion(metadata, engineInfo) || !hasSameMods(metadata, engineInfo))
	{
		// version not compatible ... ask for confirmation
		var btCaptions = [translate("Yes"), translate("No")];
		var btCode = [function(){ reallyLoadGame(gameId); }, init];
		var message = translate("This saved game may not be compatible:");
		if (!hasSameVersion(metadata, engineInfo))
			message += "\n" + sprintf(translate("It needs 0 A.D. version %(requiredVersion)s, while you are running version %(currentVersion)s."), {
				requiredVersion: metadata.version_major,
				currentVersion: engineInfo.version_major
			});

		if (!hasSameMods(metadata, engineInfo))
		{
			if (!metadata.mods)         // only for backwards compatibility with previous saved games
				metadata.mods = [];
			if (metadata.mods.length == 0)
				message += "\n" + sprintf(translate("It does not need any mod while you are running with \"%(currentMod)s\"."), {
					currentMod: engineInfo.mods.join()
				});
			else if (engineInfo.mods.length == 0)
				message += "\n" + sprintf(translate("It needs the mod \"%(requiredMod)s\" while you are running without a mod."), {
					requiredMod: metadata.mods.join()
				});
			else
				message += "\n" + sprintf(translate("It needs the mod \"%(requiredMod)s\" while you are running with \"%(currentMod)s\"."), {
					requiredMod: metadata.mods.join(),
					currentMod: engineInfo.mods.join()
				});
		}
		message += "\n" + translate("Do you still want to proceed?");
		messageBox(500, 250, message, translate("Warning"), 0, btCaptions, btCode);
	}
	else
		reallyLoadGame(gameId);
}

function reallyLoadGame(gameId)
{
	var metadata = Engine.StartSavedGame(gameId);
	if (!metadata)
	{
		// Probably the file wasn't found
		// Show error and refresh saved game list
		error(sprintf("Could not load saved game '%(id)s'", { id: gameId }));
		init();
	}
	else
	{
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": metadata.initAttributes,
			"isNetworked" : false,
			"playerAssignments": metadata.gui.playerAssignments,
			"savedGUIData": metadata.gui
		});
	}
}

function deleteGame()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");
	var gameLabel = gameSelection.list[gameSelection.selected];
	var gameID = gameSelection.list_data[gameSelection.selected];

	// Ask for confirmation
	var btCaptions = [translate("Yes"), translate("No")];
	var btCode = [function(){ reallyDeleteGame(gameID); }, null];
	messageBox(500, 200, sprintf(translate("\"%(label)s\""), { label: gameLabel }) + "\n" + translate("Saved game will be permanently deleted, are you sure?"), translate("DELETE"), 0, btCaptions, btCode);
}

function reallyDeleteGame(gameID)
{
	if (!Engine.DeleteSavedGame(gameID))
		error(sprintf("Could not delete saved game '%(id)s'", { id: gameID }));

	// Run init again to refresh saved game list
	init();
}
