function sortDecreasingDate(a, b)
{
	return b.metadata.time - a.metadata.time;
}

function twoDigits(n)
{
	return n < 10 ? "0" + n : n;
}

function generateLabel(metadata)
{
	var t = new Date(metadata.time*1000);
	// TODO: timezones
	var date = t.getUTCFullYear()+"-"+twoDigits(1+t.getUTCMonth())+"-"+twoDigits(t.getUTCDate());
	var time = twoDigits(t.getUTCHours())+":"+twoDigits(t.getUTCMinutes())+":"+twoDigits(t.getUTCSeconds());
	return "["+date+" "+time+"] "+metadata.initAttributes.map;
}

function init()
{
	var savedGames = Engine.GetSavedGames();

	var gameSelection = getGUIObjectByName("gameSelection");

	if (savedGames.length == 0)
	{
		gameSelection.list = [ "No saved games found" ];
		gameSelection.selected = 0;
		getGUIObjectByName("loadGameButton").enabled = false;
		getGUIObjectByName("deleteGameButton").enabled = false;
		return;
	}

	savedGames.sort(sortDecreasingDate);

	var gameListIDs = [ game.id for each (game in savedGames) ];
	var gameListLabels = [ generateLabel(game.metadata) for each (game in savedGames) ];

	gameSelection.list = gameListLabels;
	gameSelection.list_data = gameListIDs;
	gameSelection.selected = 0;
}

function loadGame()
{
	var gameSelection = getGUIObjectByName("gameSelection");
	var gameID = gameSelection.list_data[gameSelection.selected];

	var metadata = Engine.StartSavedGame(gameID);
	if (!metadata)
	{
		// Probably the file wasn't found
		// Show error and refresh saved game list
		error("Could not load saved game '"+gameID+"'");
		init();
	}
	else
	{
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": metadata.initAttributes,
			"isNetworked" : false,
			"playerAssignments": metadata.gui.playerAssignments
		});
	}
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
	{
		error("Could not delete saved game '"+gameID+"'");
	}

	// Run init again to refresh saved game list
	init();
}
