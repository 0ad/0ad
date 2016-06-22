var g_Descriptions;
var g_SavedGameData;

function selectDescription()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	if (gameSelection.selected == -1)
		return;

	let gameID = gameSelection.list_data[gameSelection.selected];
	Engine.GetGUIObjectByName("deleteGameButton").enabled = true;
	Engine.GetGUIObjectByName("saveGameDesc").caption = g_Descriptions[gameID];
}

function init(data)
{
	g_SavedGameData = data && data.savedGameData || {};
	let simulationState = Engine.GuiInterfaceCall("GetSimulationState");
	g_SavedGameData.timeElapsed = simulationState.timeElapsed;
	g_SavedGameData.states = [];
	for (let player of simulationState.players)
		g_SavedGameData.states.push(player.state);

	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	Engine.GetGUIObjectByName("deleteGameButton").enabled = false;

	let savedGames = Engine.GetSavedGames().sort(sortDecreasingDate);
	if (!savedGames.length)
	{
		gameSelection.list = [translate("No saved games found")];
		gameSelection.selected = -1;
		return;
	}

	g_Descriptions = {};
	for (let game of savedGames)
		g_Descriptions[game.id] = game.metadata.description || "";

	gameSelection.list = savedGames.map(game => generateLabel(game.metadata));
	gameSelection.list_data = savedGames.map(game => game.id);
	gameSelection.selected = -1;
}

function saveGame()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let gameLabel = gameSelection.list[gameSelection.selected];
	let gameID = gameSelection.list_data[gameSelection.selected];
	let desc = Engine.GetGUIObjectByName("saveGameDesc").caption;
	let name = gameID || "savegame";

	if (gameSelection.selected == -1)
	{
		reallySaveGame(name, desc, true);
		return;
	}

	messageBox(
		500, 200,
		sprintf(translate("\"%(label)s\""), { "label": gameLabel }) + "\n" +
			translate("Saved game will be permanently overwritten, are you sure?"),
		translate("OVERWRITE SAVE"),
		[translate("No"), translate("Yes")],
		[null, function(){ reallySaveGame(name, desc, false); }]
	);
}

function reallySaveGame(name, desc, nameIsPrefix)
{
	if (nameIsPrefix)
		Engine.SaveGamePrefix(name, desc, g_SavedGameData);
	else
		Engine.SaveGame(name, desc, g_SavedGameData);

	closeSave();
}

function closeSave()
{
	Engine.PopGuiPageCB(0);
}

function deleteGame()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let gameLabel = gameSelection.list[gameSelection.selected];
	let gameID = gameSelection.list_data[gameSelection.selected];

	messageBox(
		500, 200,
		sprintf(translate("\"%(label)s\""), { "label": gameLabel }) + "\n" +
			translate("Saved game will be permanently deleted, are you sure?"),
		translate("DELETE"),
		[translate("No"), translate("Yes")],
		[null, function(){ reallyDeleteGame(gameID); }]
	);
}

// HACK: Engine.SaveGame* expects this function to be defined on the current page.
// That's why we have to pass the data to this page even if we don't need it.
function getSavedGameData()
{
	return g_SavedGameData;
}
