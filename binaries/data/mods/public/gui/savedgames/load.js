var gameMetadatas = [];

function init()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");

	var savedGames = Engine.GetSavedGames();

	// get current game version and loaded mods 
	var engineInfo = Engine.GetEngineInfo();

	savedGames.sort(sortDecreasingDate);

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
		var btCaptions = ["Yes", "No"];
		var btCode = [function(){ reallyLoadGame(gameId); }, init];
		var message = "This saved game may not be compatible:";
		if (!hasSameVersion(metadata, engineInfo))
			message += "\nIt needs 0AD version " + metadata.version_major
				+ " while you are running version " + engineInfo.version_major + ".";

		if (!hasSameMods(metadata, engineInfo))
		{
			if (!metadata.mods)         // only for backwards compatibility with previous saved games
				metadata.mods = [];
			if (metadata.mods.length == 0)
				message += "\nIt does not need any mod"
					+ " while you are running with \"" + engineInfo.mods.join() + "\".";
			else if (engineInfo.mods.length == 0)
				message += "\nIt needs the mod \"" + metadata.mods.join() + "\""
					+ " while you are running without mod.";
			else
				message += "\nIt needs the mod \"" + metadata.mods.join() + "\""
					+ " while you are running with \"" + engineInfo.mods.join() + "\".";
		}
		message += "\nDo you still want to proceed ?";
		messageBox(500, 250, message, "Warning", 0, btCaptions, btCode);
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
		error("Could not load saved game '"+gameId+"'");
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
	var btCaptions = ["Yes", "No"];
	var btCode = [function(){ reallyDeleteGame(gameID); }, null];
	messageBox(500, 200, "\""+gameLabel+"\"\nSaved game will be permanently deleted, are you sure?", "DELETE", 0, btCaptions, btCode);
}

function reallyDeleteGame(gameID)
{
	if (!Engine.DeleteSavedGame(gameID))
		error("Could not delete saved game '"+gameID+"'");

	// Run init again to refresh saved game list
	init();
}
