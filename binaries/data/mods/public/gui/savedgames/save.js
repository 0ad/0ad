
var g_Descriptions;
var savedGameData;

function selectDescription()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");
	if (gameSelection.selected != -1)
	{
		Engine.GetGUIObjectByName("deleteGameButton").enabled = true;
		var gameID = gameSelection.list_data[gameSelection.selected];
		Engine.GetGUIObjectByName("saveGameDesc").caption = g_Descriptions[gameID];
	}
}

function init(data)
{
	if (data)
	{
		if (data.savedGameData)
			savedGameData = data.savedGameData;
	}

	var gameSelection = Engine.GetGUIObjectByName("gameSelection");
	Engine.GetGUIObjectByName("deleteGameButton").enabled = false;

	var savedGames = Engine.GetSavedGames();
	if (savedGames.length == 0)
	{
		gameSelection.list = [translate("No saved games found")];
		gameSelection.selected = -1;
		return;
	}

	savedGames.sort(sortDecreasingDate);

	var gameListIDs = [ game.id for each (game in savedGames) ];
	var gameListLabels = [ generateLabel(game.metadata) for each (game in savedGames) ];

	g_Descriptions = {};
	[ g_Descriptions[game.id] = (game.metadata.description ? game.metadata.description : "") for each (game in savedGames) ];

	gameSelection.list = gameListLabels;
	gameSelection.list_data = gameListIDs;
	gameSelection.selected = -1;
}

function saveGame()
{
	var gameSelection = Engine.GetGUIObjectByName("gameSelection");
	var gameLabel = gameSelection.list[gameSelection.selected];
	var gameID = gameSelection.list_data[gameSelection.selected];
	var desc = Engine.GetGUIObjectByName("saveGameDesc").caption;
	var name = gameID ? gameID : "savegame";

	if (gameSelection.selected != -1)
	{
		// Ask for confirmation
		var btCaptions = [translate("Yes"), translate("No")];
		var btCode = [function(){ reallySaveGame(name, desc, false); }, null];
		messageBox(500, 200, sprintf(translate("\"%(label)s\""), { label: gameLabel }) + "\n" + translate("Saved game will be permanently overwritten, are you sure?"), translate("OVERWRITE SAVE"), 0, btCaptions, btCode);
	}
	else
		reallySaveGame(name, desc, true);
}

function reallySaveGame(name, desc, nameIsPrefix)
{
	if (nameIsPrefix)
		Engine.SaveGamePrefix(name, desc, savedGameData);
	else
		Engine.SaveGame(name, desc, savedGameData);

	closeSave();
}

function closeSave()
{
	Engine.PopGuiPageCB(0);
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

// HACK: Engine.SaveGame* expects this function to be defined on the current page.
// That's why we have to pass the data to this page even if we don't need it.
function getSavedGameData()
{
	return savedGameData;
}
