
var g_Descriptions;
var closeCallback;
var gameDataCallback;

function selectDescription()
{
	var gameSelection = getGUIObjectByName("gameSelection");
	if (gameSelection.selected != -1)
	{
		getGUIObjectByName("deleteGameButton").enabled = true;
		var gameID = gameSelection.list_data[gameSelection.selected];
		getGUIObjectByName("saveGameDesc").caption = g_Descriptions[gameID];
	}
}

function init(data)
{
	if (data)
	{
		if (data.closeCallback)
			closeCallback = data.closeCallback;
		if (data.gameDataCallback)
			gameDataCallback = data.gameDataCallback;
	}

	var gameSelection = getGUIObjectByName("gameSelection");
	getGUIObjectByName("deleteGameButton").enabled = false;

	var savedGames = Engine.GetSavedGames();
	if (savedGames.length == 0)
	{
		gameSelection.list = [ "No saved games found" ];
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
	var gameSelection = getGUIObjectByName("gameSelection");
	var gameLabel = gameSelection.list[gameSelection.selected];
	var gameID = gameSelection.list_data[gameSelection.selected];
	var desc = getGUIObjectByName("saveGameDesc").caption;
	var name = gameID ? gameID : "savegame";

	if (gameSelection.selected != -1)
	{
		// Ask for confirmation
		var btCaptions = ["Yes", "No"];
		var btCode = [function(){ reallySaveGame(name, desc, false); }, null];
		messageBox(500, 200, "\""+gameLabel+"\"\nSaved game will be permanently overwritten, are you sure?", "OVERWRITE SAVE", 0, btCaptions, btCode);
	}
	else
		reallySaveGame(name, desc, true);
}

function reallySaveGame(name, desc, nameIsPrefix)
{
	if (nameIsPrefix)
		Engine.SaveGamePrefix(name, desc);
	else
		Engine.SaveGame(name, desc);

	closeSave();
}

function closeSave()
{
	Engine.PopGuiPage();
	if (closeCallback)
		closeCallback();
}

function deleteGame()
{
	var gameSelection = getGUIObjectByName("gameSelection");
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

// HACK: Engine.SaveGame* expects this function to be defined on the current page
function getSavedGameData()
{
	return gameDataCallback();
}
